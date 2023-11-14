#include "../builtin.h"
#include "../fdtable.h"
#include "../var.h"
#include "../exec.h"
#include "../../lib/byte.h"
#include <unistd.h>
#include <errno.h>

/* output stuff
 * ----------------------------------------------------------------------- */
int
builtin_which(int argc, char* argv[]) {
  int ret = 0, c, all = 0;
  const char* x;
  char* arg;
  stralloc p;
  size_t i, len, n;

  /* check options */
  while((c = shell_getopt(argc, argv, "a")) > 0) {
    switch(c) {
      case 'a': all = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  stralloc_init(&p);

  while((arg = argv[shell_optind++])) {
    x = var_value("PATH", &n);

    for(i = 0; i < n; i++) {
      len = byte_chr(&x[i], n - i, ':');

      stralloc_copyb(&p, &x[i], len);
      stralloc_catc(&p, '/');
      stralloc_cats(&p, arg);
      stralloc_nul(&p);

      if(access(p.s, X_OK) == 0) {
        buffer_puts(fd_out->w, p.s);
        buffer_putnlflush(fd_out->w);
        ret = 0;

        if(!all)
          break;

      } else if(!all) {
        ret = errno == ENOENT ? EXIT_NOTFOUND : EXIT_FAILURE;
        // break;
      }

      i += len;
    }

    if(ret) {
      builtin_error(argv, arg);
      break;
    }
  }

  return ret;
}
