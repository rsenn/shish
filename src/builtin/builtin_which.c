#include "../builtin.h"
#include "../fd.h"
#include "../var.h"
#include "../../lib/byte.h"
#include <unistd.h>
#include <errno.h>

/* output stuff
 * ----------------------------------------------------------------------- */
int
builtin_which(int argc, char* argv[]) {
  int ret = 0;
  const char* x;
  char* arg;
  stralloc path;
  size_t i, len, n;
  stralloc_init(&path);

  while((arg = argv[shell_optind++])) {
    x = var_value("PATH", &n);

    for(i = 0; i < n; i++) {
      len = byte_chr(&x[i], n - i, ':');

      stralloc_copyb(&path, &x[i], len);
      stralloc_catc(&path, '/');
      stralloc_cats(&path, arg);
      stralloc_nul(&path);

      if(access(path.s, X_OK) == 0) {
        buffer_puts(fd_out->w, path.s);
        buffer_putnlflush(fd_out->w);
      } else {
        ret = errno == ENOENT ? 127 : 1;
        break;
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
