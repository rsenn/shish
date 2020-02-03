#include "builtin.h"
#include "fd.h"

/* ----------------------------------------------------------------------- */
int
builtin_dirname(int argc, char** argv) {
  if(argc < 2) {
    builtin_errmsg(argv, "too few arguments", NULL);
    return 1;
  }

  buffer_puts(fd_out->w, dirname(argv[1]));
  buffer_putnlflush(fd_out->w);
  return 0;
}
