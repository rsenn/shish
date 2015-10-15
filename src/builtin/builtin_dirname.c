#include "shell.h"
#include "builtin.h"
#include "fd.h"

/* ----------------------------------------------------------------------- */
int builtin_dirname(int argc, char **argv) {
  if(!argv[shell_optind]) {
    builtin_errmsg(argv, "too few arguments", NULL);
    return 1;
  }

  buffer_puts(fd_out->w, shell_dirname(argv[shell_optind]));
  buffer_putnlflush(fd_out->w);
  return 0;
}


