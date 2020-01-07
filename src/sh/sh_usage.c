#include "fd.h"
#include "sh.h"

/* output help
 * ----------------------------------------------------------------------- */
void
sh_usage(void) {
  buffer_puts(fd_err->w, "usage: ");
  buffer_puts(fd_err->w, sh_name);
  buffer_puts(fd_err->w, " [options] [-] [script] [args]\n");
  buffer_flush(fd_err->w);
  sh_exit(1);
}
