#include "builtin_config.h"

#if BUILTIN_REALPATH

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/path.h"
#include "../../../lib/stralloc.h"
#include "../../../lib/str.h"
#include "../../../lib/byte.h"
#include <errno.h>
#include <sys/stat.h>

const char help_realpath[] = "    Print the resolved absolute pathname.\n"
                             "\n"
                             "    -E                     all but the last component must exist (default)\n"
                             "    -e                     every component must exist\n"
                             "    -q                     no diagnostics, only the exit status\n"
                             "    -L                     don't resolve symbolic links (same as -s)\n"
                             "    -P                     resolve symbolic links (default)\n"
                             "    -s, --strip            don't resolve symbolic links\n"
                             "    --relative-to=DIR      print the result relative to DIR\n"
                             "    file                   path to resolve\n";

/* does the resolved path meet mode 'e' (it exists) or 'E' (its directory exists)? */
static int
realpath_exists(const char* resolved, int mode) {
  struct stat st;
  stralloc dir;
  size_t n = str_len(resolved);
  int ok;

  if(mode == 'e')
    return stat(resolved, &st) == 0;

  n = path_dirlen(resolved, n);

  if(!n)
    return 1;

  stralloc_init(&dir);
  stralloc_copyb(&dir, resolved, n);
  stralloc_nul(&dir);
  ok = stat(dir.s, &st) == 0;
  stralloc_free(&dir);
  return ok;
}

int
builtin_realpath(int argc, char* argv[]) {
  int c, symbolic = 0, ret = 0, i, mode = 'E', quiet = 0;
  stralloc sa, base, rel;
  char* relto = NULL;

  while((c = shell_getopt_long(argc, argv, "EeLPqs", "relative-to:=R canonicalize-existing=e canonicalize-missing=E logical=L physical=P quiet=q strip=s no-symlinks=s")) > 0) {
    switch(c) {
      case 'R': relto = shell_optarg; break;
      case 'L':
      case 's': symbolic = 1; break;
      case 'P': symbolic = 0; break;
      case 'E':
      case 'e': mode = c; break;
      case 'q': quiet = 1; break;
      default: return builtin_invopt(argv);
    }
  }

  if(shell_optind >= argc) {
    builtin_errmsg(argv, "missing operand", NULL);
    return 1;
  }

  stralloc_init(&sa);
  stralloc_init(&base);
  stralloc_init(&rel);

  if(relto) {
    base.len = 0;

    if(!path_realpath(relto, &base, symbolic, NULL)) {
      builtin_error(argv, relto);
      stralloc_free(&sa);
      stralloc_free(&base);
      stralloc_free(&rel);
      return 1;
    }

    stralloc_nul(&base);
  }

  for(i = shell_optind; i < argc; i++) {
    sa.len = 0;

    if(!path_realpath(argv[i], &sa, symbolic, NULL)) {
      if(!quiet)
        builtin_error(argv, argv[i]);

      ret = 1;
      continue;
    }

    stralloc_nul(&sa);

    if(!realpath_exists(sa.s, mode)) {
      errno = ENOENT;

      if(!quiet)
        builtin_error(argv, argv[i]);

      ret = 1;
      continue;
    }

    if(relto) {
      path_rel_sa(&rel, sa.s, sa.len, base.s, base.len);
      buffer_puts(fd_out->w, rel.s);
    } else {
      buffer_puts(fd_out->w, sa.s);
    }

    buffer_putnlflush(fd_out->w);
  }

  stralloc_free(&sa);
  stralloc_free(&base);
  stralloc_free(&rel);
  return ret;
}
#endif /* BUILTIN_REALPATH */
