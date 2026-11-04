#include "builtin_config.h"

#if BUILTIN_CHMOD || BUILTIN_MKDIR

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../sh.h"
#include "../../../lib/shell.h"
#include "../../../lib/scan.h"
#include "../../../lib/str.h"
#include "../../../lib/path.h"
#include "../../../lib/byte.h"
#include "../../../lib/walk.h"
#include "../../../lib/unix.h"
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

/* true if every character of s is one of set */
static int
chmod_only(const char* s, const char* set) {
  for(; *s; s++)
    if(!set[str_chr(set, *s)])
      return 0;

  return 1;
}

/* apply a comma-separated symbolic mode spec ("[augo]*([+-=][rwxXst]*|[+-=][ugo])+",
 * e.g. "u+rwx,go-w", "g=u", "+t") on top of an existing mode. Returns 1 on
 * success, 0 if spec doesn't parse.
 *   - an omitted who is "a" masked by the umask; "s" then means setuid+setgid
 *   - "s" sets setuid for u, setgid for g; "t" sets the sticky bit
 *   - "=u", "+g", "-o" use the *current* permissions of that class
 *
 *   const char*    spec    comma-separated symbolic clauses to apply
 *   unsigned int*  mode    in/out: mode to modify; "X" reads its incoming value
 *   int            is_dir  whether the target is a directory, for "X" handling
 * ----------------------------------------------------------------------- */
int
chmod_symbolic(const char* spec, unsigned int* mode, int is_dir) {
  const char* p = spec;
  unsigned int orig_mode = *mode;

  if(!*p)
    return 0;

  while(*p) {
    unsigned int who = 0, mask, clear;
    int explicit_who;

    for(; *p == 'u' || *p == 'g' || *p == 'o' || *p == 'a'; p++) {
      switch(*p) {
        case 'u': who |= 0700; break;
        case 'g': who |= 0070; break;
        case 'o': who |= 0007; break;
        case 'a': who |= 0777; break;
      }
    }

    explicit_who = who != 0;
    mask = who;

    if(*p != '+' && *p != '-' && *p != '=')
      return 0;

    while(*p == '+' || *p == '-' || *p == '=') {
      char op = *p++;
      unsigned int perm = 0, special = 0;

      /* an omitted who is "a" limited by the umask, "-w" included */
      if(!explicit_who)
        mask = 0777 & ~(unsigned int)sh->umask;

      if(*p == 'u' || *p == 'g' || *p == 'o') {
        /* copy another class's rwx triple into every class of mask */
        unsigned int t = (*mode >> (*p == 'u' ? 6 : *p == 'g' ? 3 : 0)) & 7;

        perm = (t << 6 | t << 3 | t) & mask;
        p++;
      } else {
        for(; *p == 'r' || *p == 'w' || *p == 'x' || *p == 'X' || *p == 's' || *p == 't'; p++) {
          switch(*p) {
            case 'r': perm |= 0444; break;
            case 'w': perm |= 0222; break;
            case 'x': perm |= 0111; break;
            case 'X':
              if(is_dir || (orig_mode & 0111))
                perm |= 0111;
              break;
            case 's':
              if(!explicit_who || (who & 0700))
                special |= 04000;
              if(!explicit_who || (who & 0070))
                special |= 02000;
              break;
            case 't':
              if(!explicit_who || (who & 0007))
                special |= 01000;
              break;
          }
        }

        perm &= mask;
      }

      switch(op) {
        case '+': *mode |= perm | special; break;
        case '-': *mode &= ~(perm | special); break;
        case '=':
          /* "=" also drops the special bits that belong to the classes it names */
          clear = explicit_who ? who : 0777;
          *mode = (*mode & ~(clear | ((clear & 0700) ? 04000 : 0) | ((clear & 0070) ? 02000 : 0) | ((clear & 0007) ? 01000 : 0))) |
                  perm | special;
          break;
      }
    }

    if(*p == ',') {
      p++;
      continue;
    }

    break;
  }

  return *p == '\0';
}

/* what chmod was asked to do, for chmod_tree() */
struct chmod_opt {
  char** argv;             /* for error reporting */
  const char* spec;        /* octal or symbolic mode spec */
  unsigned int octal_mode; /* parsed spec, when !symbolic */
  unsigned symbolic : 1;   /* spec is symbolic, applied to each file's own mode */
  unsigned force : 1;      /* suppress errors, keep processing siblings */
  unsigned verbose : 1;    /* print a line for every file processed */
  unsigned changes : 1;    /* print a line only for files whose mode changed */
};

/* apply the mode to one path: stat() supplies the dereferenced current mode a
 * symbolic spec applies on top of, matching chmod(2) itself.
 * Returns 0, 1 if chmod(2) failed, -1 if nothing was attempted (already reported).
 * ----------------------------------------------------------------------- */
static int
chmod_one(const struct chmod_opt* o, const char* path) {
  struct stat st;
  unsigned int old_mode, mode;
  int ret = 0;

  if(stat(path, &st) == -1) {
    if(!o->force)
      builtin_error(o->argv, (char*)path);
    return -1;
  }

  old_mode = st.st_mode & 07777;
  mode = o->symbolic ? old_mode : o->octal_mode;

  if(o->symbolic && !chmod_symbolic(o->spec, &mode, S_ISDIR(st.st_mode))) {
    if(!o->force)
      builtin_errmsg(o->argv, (char*)o->spec, "invalid mode");
    return -1;
  }

  if(chmod(path, mode) == -1) {
    if(!o->force)
      builtin_error(o->argv, (char*)path);
    ret = 1;
  } else if((o->verbose || o->changes) && (!o->changes || mode != old_mode)) {
    buffer_putm_internal(fd_out->w, "changed mode of '", path, "'", 0);
    buffer_putnlflush(fd_out->w);
  }

  return ret;
}

/* chmod an operand and, with 'recursive', everything below it, directory first.
 * The walk is physical: a symlink named on the command line is chmoded (and
 * dereferenced by chmod(2)) but not entered, unless written "link/"; one met while recursing is left
 * untouched entirely (matching GNU chmod). Returns 1 if anything failed.
 * ----------------------------------------------------------------------- */
static int
chmod_tree(const struct chmod_opt* o, const char* root, int recursive) {
  struct walk w;
  struct walk_ent* e;
  size_t n = str_len(root);
  int ret = 0, r;

  /* "link/" names the directory behind the link, as the kernel resolves it:
     only then is the root dereferenced */
  if(walk_open(&w, root, WALK_PRE | WALK_ERR | (n && root[n - 1] == '/' ? WALK_COMFOLLOW : 0))) {
    builtin_error(o->argv, (char*)root);
    return 1;
  }

  if(!recursive)
    w.maxdepth = 0;

  while((e = walk_read(&w))) {
    if(e->phase == WALK_ERR) {
      if(!o->force) {
        errno = e->err;
        builtin_error(o->argv, (char*)e->path);
      }
      ret = 1;
      continue;
    }

    if(e->depth && e->islink) {
      if(o->verbose) {
        buffer_putm_internal(fd_out->w, "neither symbolic link '", e->path, "' nor referent has been changed", 0);
        buffer_putnlflush(fd_out->w);
      }
      continue;
    }

    if((r = chmod_one(o, e->path))) {
      ret = 1;

      /* nothing was changed here, so nothing below is attempted either */
      if(r < 0 && e->phase == WALK_PRE)
        walk_skip(&w);
    }
  }

  walk_close(&w);
  return ret;
}

/* output stuff
 * ----------------------------------------------------------------------- */
const char help_chmod[] = "    Change file permissions.\n"
                          "\n"
                          "    -v              print a line for each file whose mode changed\n"
                          "    -c              like -v, but only for files whose mode actually changed\n"
                          "    -f              suppress most error messages\n"
                          "    -R              change files and directories recursively\n"
                          "    mode            octal number, or symbolic \"[ugoa]+-=[rwxX]\",\n"
                          "                    comma-separated clauses applied to each file's\n"
                          "                    current mode (e.g. \"u+rwx,go-w\")\n"
                          "    file            file(s) to change the mode of\n"
                          "\n"
                          "    long options: --recursive -R, --verbose -v, --changes -c, --silent -f,\n"
                          "                  --quiet -f\n";

int
builtin_chmod(int argc, char* argv[]) {
  int c;
  int recursive = 0;
  int failed = 0;
  char *p, *spec, *premode = NULL;
  struct chmod_opt o = {argv, 0, 0, 0, 0, 0, 0};

  /* "chmod -w f": a leading-dash word made only of mode characters is the
     mode, not an option; cut it out of argv before option parsing */
  for(c = 1; c < argc && argv[c][0] == '-' && argv[c][1] && str_diff(argv[c], "--"); c++) {
    const char* q = argv[c] + 1;

    if(chmod_only(q, "rwxXstugoa+-=,") && !chmod_only(q, "vcfR")) {
      premode = argv[c];
      byte_copyr(&argv[c], (argc - c) * sizeof(char*), &argv[c + 1]);
      argc--;
      break;
    }
  }

  /* check options */
  while((c = shell_getopt_long(argc, argv, "vcfR", "recursive=R verbose=v changes=c silent=f quiet=f")) > 0) {
    switch(c) {
      case 'v': o.verbose = 1; break;
      case 'c': o.changes = 1; break;
      case 'f': o.force = 1; break;
      case 'R': recursive = 1; break;
      default: return builtin_invopt(argv);
    }
  }

  if(!argv[shell_optind]) {
    builtin_errmsg(argv, "missing operand", NULL);
    return 1;
  }

  spec = premode ? premode : argv[shell_optind++];

  if(!argv[shell_optind]) {
    builtin_errmsg(argv, "missing operand after mode", NULL);
    return 1;
  }

  /* a mode made up entirely of octal digits is the plain numeric
     form; anything else must parse as symbolic mode instead, applied
     relative to each file's own current mode */
  o.spec = spec;
  o.symbolic = scan_8int(spec, &o.octal_mode) != str_len(spec);

  while((p = argv[shell_optind++])) {
    if(chmod_tree(&o, p, recursive)) {
      failed = 1;

      if(!o.force)
        return 1;
    }
  }

  return failed;
}
#endif /* BUILTIN_CHMOD || BUILTIN_MKDIR */
