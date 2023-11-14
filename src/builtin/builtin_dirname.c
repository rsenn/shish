#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/str.h"

/* ----------------------------------------------------------------------- */
int
builtin_dirname(int argc, char* argv[]) {
  char* path;
  int i;

  if(argc < 2) {
    builtin_errmsg(argv, "too few arguments", NULL);
    return 1;
  }

  path = argv[1];

  if(path) {
    for(i = str_len(path) - 1; i >= 0; i--) {
      if(path[i] == '/') {
        path[i] = '\0';
        break;
      }
    }

    if(i < 0) {
      path[0] = '.';
      path[1] = '\0';
    }
  }

  buffer_puts(fd_out->w, path);
  buffer_putnlflush(fd_out->w);
  return 0;
}
