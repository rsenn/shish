#include "../builtin.h"
#include "../fdtable.h"
#include "../exec.h"
#include "../../lib/alloc.h"
#include "../../lib/fmt.h"
#include "../../lib/buffer.h"
#include "../../lib/str.h"

/* print the "hits    command" table (no options / default output)
 * ----------------------------------------------------------------------- */
static void
hash_print_table(void) {
  unsigned int i, n;
  struct exechash* h;
  char num[FMT_ULONG];

  buffer_puts(fd_out->w, "hits    command\n");

  for(i = 0; i < EXEC_HASHSIZE; i++) {
    for(h = exec_hashtbl[i]; h; h = h->next) {
      /* print hits right-aligned */
      n = fmt_ulong(num, h->hits);
      num[n] = '\0';
      buffer_putnspace(fd_out->w, (n = 4 - n));
      buffer_puts(fd_out->w, num);

      /* print command left aligned in next column */
      buffer_putnspace(fd_out->w, 4 + (n > 0 ? 0 : n));

      switch(h->cmd.id) {
        case H_PROGRAM: buffer_puts(fd_out->w, h->cmd.path); break;
        case H_EXEC:
        case H_SBUILTIN:
          buffer_putm_internal(fd_out->w, h->name, " (special builtin)", NULL);
          break;
        case H_BUILTIN: buffer_putm_internal(fd_out->w, h->name, " (builtin)", NULL); break;
        case H_FUNCTION: buffer_putm_internal(fd_out->w, h->name, " (function)", NULL); break;
      }

      buffer_putnlflush(fd_out->w);
    }
  }
}

/* -l: print in a form that can be fed straight back in as a script to
 * recreate the table -- only H_PROGRAM entries have a pathname "-p"
 * can reuse, so entries hashed as a builtin/function/special builtin
 * are skipped (there's no "-p"-shaped way to re-hash those anyway).
 * ----------------------------------------------------------------------- */
static void
hash_print_reusable(void) {
  unsigned int i;
  struct exechash* h;

  for(i = 0; i < EXEC_HASHSIZE; i++)
    for(h = exec_hashtbl[i]; h; h = h->next)
      if(h->cmd.id == H_PROGRAM)
        buffer_putm_internal(fd_out->w, "hash -p ", h->cmd.path, " ", h->name, "\n", NULL);

  buffer_flush(fd_out->w);
}

/* -r: forget every remembered command location. Unlike
 * exec_hash_invalidate_all() (exec_hash.c, private to that file --
 * used internally to force a re-search after PATH changes, keeping
 * each entry's slot around for reuse), this actually frees every
 * entry, matching "forget", not just "distrust", every remembered
 * location.
 * ----------------------------------------------------------------------- */
static void
hash_forget_all(void) {
  unsigned int i;
  struct exechash *h, *next;

  for(i = 0; i < EXEC_HASHSIZE; i++) {
    for(h = exec_hashtbl[i]; h; h = next) {
      next = h->next;

      if(h->cmd.id == H_PROGRAM)
        alloc_free(h->cmd.path);

      alloc_free(h->name);
      alloc_free(h);
    }

    exec_hashtbl[i] = NULL;
  }
}

/* -d name: forget one remembered command location
 * ----------------------------------------------------------------------- */
static int
hash_forget(char* name) {
  uint32 hash = exec_hashstr(name);
  struct exechash **hp, *h;

  for(hp = &exec_hashtbl[hash & EXEC_HASHMASK]; (h = *hp); hp = &h->next) {
    if(h->hash == hash && !str_diff(h->name, name)) {
      *hp = h->next;

      if(h->cmd.id == H_PROGRAM)
        alloc_free(h->cmd.path);

      alloc_free(h->name);
      alloc_free(h);
      return 1;
    }
  }

  return 0;
}

/* -p pathname name: remember 'name' as located at 'pathname', without
 * searching PATH for it at all -- same as bash's "-p", this is taken
 * on faith; a wrong pathname just fails the next time 'name' actually
 * runs. Overwrites any existing entry for 'name' (of any kind, not
 * just a previous H_PROGRAM one).
 * ----------------------------------------------------------------------- */
static void
hash_set(char* name, char* pathname) {
  /* exec_lookup() only writes back through its hashptr param on a
     cache *hit* -- on a miss (the common case here, hashing a name
     for the first time) it leaves it untouched, so it has to be
     pre-computed here rather than left for exec_lookup() to maybe
     fill in, or exec_create() below ends up hashing/bucketing the new
     entry by a garbage value nothing will ever look up correctly. */
  uint32 hash = exec_hashstr(name);
  struct exechash* entry = exec_lookup(name, &hash);

  if(!entry)
    entry = exec_create(name, hash);
  else if(entry->cmd.id == H_PROGRAM)
    alloc_free(entry->cmd.path);

  entry->cmd.id = H_PROGRAM;
  entry->cmd.path = str_dup(pathname);
  entry->mask = 0;
  entry->hits = 0;
}

/* hash built-in
 *
 * usage: hash [-lr] [-p pathname name] [-d name ...] [name ...]
 * ----------------------------------------------------------------------- */
int
builtin_hash(int argc, char* argv[]) {
  int i = 1, ret = 0;
  int opt_r = 0, opt_l = 0, opt_d = 0, opt_p = 0;
  char* pathname = NULL;

  for(; i < argc && argv[i][0] == '-' && argv[i][1]; i++) {
    switch(argv[i][1]) {
      case 'r': opt_r = 1; break;
      case 'l': opt_l = 1; break;
      case 'd': opt_d = 1; break;

      case 'p':
        opt_p = 1;

        if(argv[i][2])
          pathname = &argv[i][2];
        else if(i + 1 < argc)
          pathname = argv[++i];
        else
          return builtin_errmsg(argv, "-p", "option requires an argument");

        break;

      default: return builtin_invopt(argv);
    }
  }

  if(opt_r) {
    hash_forget_all();
    return 0;
  }

  if(opt_p) {
    if(i >= argc)
      return builtin_errmsg(argv, "-p", "requires a name argument");

    for(; i < argc; i++)
      hash_set(argv[i], pathname);

    return 0;
  }

  if(opt_d) {
    if(i >= argc)
      return builtin_errmsg(argv, "-d", "requires a name argument");

    for(; i < argc; i++) {
      if(!hash_forget(argv[i])) {
        builtin_errmsg(argv, argv[i], "not found");
        ret = 1;
      }
    }

    return ret;
  }

  if(opt_l) {
    hash_print_reusable();
    return 0;
  }

  /* bare names (no options): (re-)hash each, same as the shell would
     do the first time it tries to run them, without actually running
     anything */
  if(i < argc) {
    for(; i < argc; i++) {
      struct command cmd = exec_hash(argv[i], 0);

      if(!cmd.ptr) {
        builtin_errmsg(argv, argv[i], "not found");
        ret = 1;
      }
    }

    return ret;
  }

  hash_print_table();

  return 0;
}
