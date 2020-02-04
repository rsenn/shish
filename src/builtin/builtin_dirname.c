#include "builtin.h"
#include "fd.h"

/* ----------------------------------------------------------------------- */
int
builtin_dirname(int argc, char** argv) {
  char* path;

  if(argc < 2) {
    builtin_errmsg(argv, "too few arguments", NULL);
    return 1;
  }

  path = argv[1];
  if(path) {
    int i;
    for(i = str_len(path) - 1; i >= 0; i--) {
      if(path[i] == '/') {
        path[i] = '\0';
        break;
      }
    }
  }

  buffer_puts(fd_out->w, path);
  buffer_putnlflush(fd_out->w);
  return 0;
}
