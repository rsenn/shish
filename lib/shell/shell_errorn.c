#include "shell.h"
#include <string.h>
#include <errno.h>

/* output error message
 * ----------------------------------------------------------------------- */
int shell_errorn(const char *s, unsigned int len)
{
  buffer_put(shell_buff, s, len);
  buffer_putm(shell_buff, ": ", strerror(errno), "\n", NULL);
  buffer_flush(shell_buff);
  return 1;
}

