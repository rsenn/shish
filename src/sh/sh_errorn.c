#include <string.h>
#include <errno.h>
#include <shell.h>
#include "fd.h"
#include "sh.h"

/* output error message
 * ----------------------------------------------------------------------- */
int sh_errorn(const char *s, unsigned int len)
{
  sh_msg(NULL);
  buffer_put(fd_err->w, s, len);
  buffer_putm(fd_err->w, ": ", strerror(errno), "\n", NULL);
  buffer_flush(fd_err->w);
  return 1;
}

