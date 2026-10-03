#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../sh.h"
#include "../../../lib/shell.h"
#include "../../../lib/scan.h"
#include "../../../lib/str.h"
#include "../../../lib/byte.h"
#include "../../../lib/stralloc.h"
#include "../../../lib/unix.h"
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
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

/* apply mode (octal, or symbolic relative to each path's own current
 * mode) to a single path, recursing into directories first if
 * 'recursive' is set.
 * - lstat() decides whether to recurse, so a symlink to a directory
 *   is changed but not traversed into (matching builtin_rm's
 *   loop-avoidance).
 * - stat() supplies the dereferenced current mode a symbolic spec
 *   applies on top of, matching chmod(2) itself.
 * - with 'force' set, errors are not reported (but still counted),
 *   and sibling entries are still processed.
 * - 'toplevel' distinguishes a command-line operand from an entry
 *   found while recursing: a symlink merely encountered during
 *   recursion is left untouched entirely (matching GNU chmod), while
 *   one named directly is still dereferenced and chmoded.
 *
 *   char*[]        argv         argv, for error reporting
 *   stralloc*      path         path to chmod; reused as scratch space when recursing
 *   char*          spec         octal or symbolic mode spec
 *   int            symbolic     whether spec is symbolic (vs. plain octal)
 *   unsigned int   octal_mode   parsed octal mode, when !symbolic
 *   int            recursive    recurse into directories
 *   int            force        suppress errors, keep processing siblings
 *   int            verbose      print a line for every file processed
 *   int            changes      print a line only for files whose mode changed
 *   int            toplevel     path is a command-line operand, not found by recursion
 * ----------------------------------------------------------------------- */
static int
chmod_path(char* argv[],
           stralloc* path,
           char* spec,
           int symbolic,
           unsigned int octal_mode,
           int recursive,
           int force,
           int verbose,
           int changes,
           int toplevel) {
  struct stat lst, st;
  unsigned int old_mode, mode;
  int ret = 0;

  if(lstat(path->s, &lst) == -1) {
    if(!force)
      builtin_error(argv, path->s);
    return 1;
  }

  if(!toplevel && S_ISLNK(lst.st_mode)) {
    if(verbose) {
      buffer_putm_internal(fd_out->w, "neither symbolic link '", path->s, "' nor referent has been changed", 0);
      buffer_putnlflush(fd_out->w);
    }
    return 0;
  }

  if(stat(path->s, &st) == -1) {
    if(!force)
      builtin_error(argv, path->s);
    return 1;
  }

  old_mode = st.st_mode & 07777;
  mode = symbolic ? old_mode : octal_mode;

  if(symbolic && !chmod_symbolic(spec, &mode, S_ISDIR(st.st_mode))) {
    if(!force)
      builtin_errmsg(argv, spec, "invalid mode");
    return 1;
  }

  if(chmod(path->s, mode) == -1) {
    if(!force)
      builtin_error(argv, path->s);
    ret = 1;
  } else if((verbose || changes) && (!changes || mode != old_mode)) {
    buffer_putm_internal(fd_out->w, "changed mode of '", path->s, "'", 0);
    buffer_putnlflush(fd_out->w);
  }

  if(recursive && S_ISDIR(lst.st_mode)) {
    DIR* dp;
    struct dirent* de;
    size_t dirlen = path->len;

    if(!(dp = opendir(path->s))) {
      if(!force)
        builtin_error(argv, path->s);
      return 1;
    }

    while((de = readdir(dp))) {
      if(!str_diff(de->d_name, ".") || !str_diff(de->d_name, ".."))
        continue;

      path->len = dirlen;
      stralloc_catc(path, '/');
      stralloc_cats(path, de->d_name);
      stralloc_nul(path);

      if(chmod_path(argv, path, spec, symbolic, octal_mode, recursive, force, verbose, changes, 0))
        ret = 1;
    }

    closedir(dp);
    path->len = dirlen;
    stralloc_nul(path);
  }

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
                          "    file            file(s) to change the mode of\n";

int
builtin_chmod(int argc, char* argv[]) {
  int c;
  int verbose = 0, changes = 0, force = 0, recursive = 0;
  int failed = 0;
  char *p, *spec, *premode = NULL;
  unsigned int octal_mode = 0;
  int symbolic;
  stralloc path;

  /* "chmod -w f": a leading-dash word made only of mode characters is the
     mode, not an option; cut it out of argv before option parsing */
  for(c = 1; c < argc && argv[c][0] == '-' && argv[c][1] && str_diff(argv[c], "--"); c++) {
    const char* q = argv[c] + 1;

    if(chmod_only(q, "rwxXstugoa+-=,") && !chmod_only(q, "vcfR")) {
      premode = argv[c];
      byte_copy(&argv[c], (argc - c) * sizeof(char*), &argv[c + 1]);
      argc--;
      break;
    }
  }

  /* check options */
  while((c = shell_getopt(argc, argv, "vcfR")) > 0) {
    switch(c) {
      case 'v': verbose = 1; break;
      case 'c': changes = 1; break;
      case 'f': force = 1; break;
      case 'R': recursive = 1; break;
      default: builtin_invopt(argv); return 1;
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
  symbolic = scan_8int(spec, &octal_mode) != str_len(spec);

  stralloc_init(&path);

  while((p = argv[shell_optind++])) {
    stralloc_copys(&path, p);
    stralloc_nul(&path);

    if(chmod_path(argv, &path, spec, symbolic, octal_mode, recursive, force, verbose, changes, 1)) {
      failed = 1;

      if(!force) {
        stralloc_free(&path);
        return 1;
      }
    }
  }

  stralloc_free(&path);
  return failed;
}
