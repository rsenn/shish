#include "builtin.h"
#include "byte.h"
#include "fd.h"
#include "sh.h"
#include "shell.h"
#include "str.h"
#include "var.h"
#include <errno.h>
#include <limits.h>
#include <string.h>
#include "windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* change working directory
 * ----------------------------------------------------------------------- */
int
builtin_cd(int argc, char** argv) {
  int c;
  int ok = 0;
  int symbolic = 1;
  const char* arg;
  size_t len, n;
  stralloc newcwd;

  /* check options, -L for symlink, -P for physical path */
  while((c = shell_getopt(argc, argv, "LP")) > 0) {
    switch(c) {
      case 'L': symbolic = 1; break;
      case 'P': symbolic = 0; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  arg = argv[shell_optind];
  stralloc_init(&newcwd);

  /* empty argument means chdir(HOME) */
  if(arg == NULL) {
    arg = var_value("HOME", &len);

    if(arg[0] == '\0') {
      sh_msg("HOME variable not set!");
      return 1;
    }
  }

  len = str_len(arg);

  /* when it isn't an absolute path we have to check CDPATH */
  if(arg[0] != '/') {
    char path[PATH_MAX + 1];
    const char* cdpath;

    /* loop through colon-separated CDPATH variable */
    cdpath = var_value("CDPATH", NULL);

    do {
      /* too much, too much :) */
      if((n = str_chr(cdpath, ':')) + len + 1 > PATH_MAX) {
        /* set error code and print the longer string in the error msg */
        errno = ENAMETOOLONG;
        return builtin_errmsgn(argv, (n > len ? cdpath : arg), (n > len ? n : len), strerror(errno));
      }

      /* copy path prefix from cdpath if present */
      if(n) {
        byte_copy(path, n, cdpath);
        cdpath += n;
        path[n++] = '/';
      }

      /* copy the argument and canonicalize */
      str_copy(&path[n], arg);

      ok = shell_realpath(path, &newcwd, symbolic, &sh->cwd);

      /* skip the colon */
      if(*cdpath == ':')
        cdpath++;
    } while(*cdpath && !ok);

  }
  /* absolute path */
  else {
    /* last cdpath length set to 0, because we're not using cdpath here */
    n = 0;
    ok = shell_canonicalize(arg, &newcwd, symbolic);
  }

  stralloc_nul(&newcwd);

  /* try to chdir() if everything's ok */
  if(ok && chdir(newcwd.s) == 0) {
    /* print path if prefix was taken from cdpath */
    if(n) {
      buffer_putsa(fd_out->w, &newcwd);
      buffer_putnlflush(fd_out->w);
    }

    /* set the path */
    stralloc_move(&sh->cwd, &newcwd);

    /* if the path has symlinks then set sh->cwdsym */
    sh->cwdsym = (ok > 1);

    return 0;
  }

  /* we failed */
  builtin_error(argv, newcwd.s);
  stralloc_free(&newcwd);
  return 1;
}
