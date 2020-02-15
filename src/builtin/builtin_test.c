#include "../builtin.h"
#include "../fd.h"
#include "../../lib/fmt.h"
#include "../../lib/shell.h"
#include "../../lib/str.h"
#include <sys/stat.h>
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#ifndef R_OK
#define R_OK 4
#endif /* defined(R_OK) */
#ifndef W_OK
#define W_OK 2
#endif /* defined(W_OK) */
#ifndef X_OK
#define X_OK 1
#endif /* defined(X_OK) */
#ifndef S_IFMT
#define S_IFMT 0xf000
#endif
#ifndef S_IFBLK
#define S_IFBLK 0x6000
#endif /* defined(S_IFBLK) */
#else
#include <unistd.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#ifndef HAVE_LSTAT
#define lstat stat
#endif
#endif

/* test for expression
 * ----------------------------------------------------------------------- */
int
builtin_test(int argc, char** argv) {
  int c, ret = -1;
  int neg = 0;
  int brackets = 0;
  struct stat st;

  if(argv[0][0] == '[') {
    brackets = 1;
    argc--;

    if(argv[argc][0] != ']') {
      builtin_errmsg(argv, "missing ]'", NULL);
      return 2;
    }
  }

  /* TODO:*/
  (void)brackets;

  /* every condition can be negated by a leading ! */
  while(shell_optind < argc && argv[shell_optind][0] == '!' && argv[shell_optind][1] == '\0') {
    neg = !neg;
    shell_optind++;
  }

  /* check options */
  while((c = shell_getopt(argc, argv, "n:z:f:d:b:c:h:L:S:e:s:r:w:x:")) > 0) {
    switch(c) {
      /* return true if argument is non-zero */
      case 'n': ret = neg ^ (!*shell_optarg); break;

      /* return true if argument is zero */
      case 'z': ret = neg ^ (!!*shell_optarg); break;

      /* return true if argument is a regular file */
      case 'f': ret = neg ^ !(stat(argv[shell_optind], &st) == 0 && S_ISREG(st.st_mode)); break;
      /* return true if argument is a directory */
      case 'd': ret = neg ^ !(stat(argv[shell_optind], &st) == 0 && S_ISDIR(st.st_mode)); break;
      /* return true if argument is a character device */
      case 'c': ret = neg ^ !(stat(argv[shell_optind], &st) == 0 && S_ISCHR(st.st_mode)); break;
      /* return true if argument is a block device */
      case 'b': ret = neg ^ !(stat(argv[shell_optind], &st) == 0 && (st.st_mode & S_IFMT) == S_IFBLK); break;
      /* return true if argument is a fifo */
      case 'p': ret = neg ^ !(stat(argv[shell_optind], &st) == 0 && S_ISFIFO(st.st_mode)); break;
      /* return true if argument is a symbolic link */
      case 'h':
#ifdef S_ISLNK
      case 'L': ret = neg ^ !(lstat(argv[shell_optind], &st) == 0 && S_ISLNK(st.st_mode));
#endif
        break;
#ifdef S_ISSOCK
      /* return true if argument is a socket */
      case 'S': ret = neg ^ !(stat(argv[shell_optind], &st) == 0 && S_ISSOCK(st.st_mode));
#endif
        break;
      /* return true if argument exists */
      case 'e': ret = neg ^ !(stat(argv[shell_optind], &st) == 0); break;
      /* return true if argument exists and is not empty */
      case 's': ret = neg ^ !(stat(argv[shell_optind], &st) == 0 && st.st_size); break;

      /* return true if readable */
      case 'r': ret = neg ^ !!access(argv[shell_optind], R_OK); break;
      /* return true if writeable */
      case 'w': ret = neg ^ !!access(argv[shell_optind], W_OK); break;
      /* return true if executable */
      case 'x': ret = neg ^ !!access(argv[shell_optind], X_OK); break;

      default: builtin_invopt(argv); return 1;
    }
  }

  /* we cannot have more than 3 arguments */
  if(argc > 4) {
    builtin_errmsg(argv, "too many arguments", NULL);
    return 2;
  }

  /* if we have 3 arguments it should be something like STRING1 EXPR STRING2 */
  if(argc - shell_optind == 3) {
    /* if operator doesn't start with '-' then its surely a string comparision */
    if(argv[2][0] != '-') {
      int ret = str_diff(argv[1], argv[3]);

      switch(argv[2][0]) {
        case '=': ret = neg ^ (!!ret);
        case '!': ret = neg ^ (!ret);
        case '<': ret = neg ^ (ret >= 0);
        case '>': ret = neg ^ (ret <= 0);
      }
    }
  }

  if(argc - shell_optind == 0)
    ret = neg ^ (!(argv[shell_optind]));

  if(ret == -1) {
    builtin_errmsg(argv, "invalid expression", argv[1]);
    return 1;
  }
  return ret;
}
