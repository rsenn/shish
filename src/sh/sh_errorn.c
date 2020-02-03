#include "fd.h"
#include "sh.h"
#include "shell.h"
#include <errno.h>
#include <string.h>

/* output error message
 * ----------------------------------------------------------------------- */
int
sh_errorn(const char* s, unsigned int len) {
  sh_msg(NULL);
  buffer_put(fd_err->w, s, len);
  buffer_puts(fd_err->w, ": ");
  buffer_puts(fd_err->w, strerror(errno));
  buffer_putnlflush(fd_err->w);
  return 1;
}
