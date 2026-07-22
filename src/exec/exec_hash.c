#include "../builtin.h"
#include "../exec.h"
#include "../tree.h"
#include "../../lib/str.h"
#include "../../lib/alloc.h"
#include "../vartab.h"
#include "../var.h"
#include "../../lib/windoze.h"
#include <errno.h>
#if WINDOWS_NATIVE
#include <io.h>
#ifndef X_OK
#define X_OK 1
#endif
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

/* POSIX 3.9.1.1 requires a PATH reassignment to invalidate any
 * remembered command locations. Every hashed H_PROGRAM entry's
 * cmd.path was resolved against whatever PATH happened to be in
 * effect at the time it was cached; without this, a later PATH
 * reassignment left it pointing at whatever the old PATH found,
 * possibly the wrong file (or no longer finding one it should).
 * Detected lazily by comparing PATH's current value against the last
 * one seen, on every exec_hash() call, rather than hooking every
 * var_set*() call site that could touch PATH (there are several:
 * plain assignment, export, local, read...) -- this way exec_hash()
 * alone owns staleness detection for its own cache.
 * ----------------------------------------------------------------------- */
static char* exec_hash_path_seen = NULL;

static void
exec_hash_invalidate_all(void) {
  unsigned int i;
  struct exechash* h;

  for(i = 0; i < EXEC_HASHSIZE; i++)
    for(h = exec_hashtbl[i]; h; h = h->next)
      h->mask = -1; /* force exec_search() re-run on next lookup, same
                        trick eval_function.c uses for a redefined
                        function's single cache entry */
}

/* hashed command search routine
 * ----------------------------------------------------------------------- */
struct command
exec_hash(char* name, int mask) {
  struct command cmd = {0, {0}};
  const char* path = var_value("PATH", NULL);

  if(!exec_hash_path_seen || str_diff(exec_hash_path_seen, path ? path : "")) {
    exec_hash_invalidate_all();
    alloc_free(exec_hash_path_seen);
    exec_hash_path_seen = str_dup(path ? path : "");
  }

  /* name contains slashes, its a path */
  if(name[str_chr(name, '/')]) {
    /* do not hash this... */
    cmd.id = H_PROGRAM;
    cmd.path = name;

    /* ...but validate the path. access(X_OK) alone accepts directories
       too (they're "executable" as in searchable) -- reject those
       explicitly, matching what execve() itself would report */
    if(access(cmd.path, X_OK) != 0) {
      exec_lasterrno = errno;
      cmd.path = NULL;
    }
#if !WINDOWS_NATIVE
    else {
      struct stat st;

      if(stat(cmd.path, &st) == 0 && S_ISDIR(st.st_mode)) {
        exec_lasterrno = EISDIR;
        cmd.path = NULL;
      }
    }
#endif

  } else {
    /* otherwise try to find hashed entry */
    struct exechash* entry;
    uint32 hash;

    /* hash the name for possible re-use on exechash_create() */
    hash = exec_hashstr(name);

    /* do we have a cache hit? */
    if((entry = exec_lookup(name, &hash))) {
      entry->hits++;
      cmd = entry->cmd;
    }

    if(!entry || entry->mask != mask) {
      /* if we don't have a cache hit we're gonna search, special builtins first
       */
      cmd = exec_search(name, mask);
    }

    if(entry && entry->mask != mask) {
      if(entry->cmd.id == H_PROGRAM)
        alloc_free(entry->cmd.path);
      entry->hits = 1;
    }

    /* if we found something then create a new cache entry */
    if(cmd.ptr && !entry) {
      entry = exec_create(name, hash);
      entry->mask = mask;
      entry->hits = 1;
    }

    if(entry) {
      entry->cmd = cmd;
      entry->mask = mask;
    }
  }

  return cmd;
}
