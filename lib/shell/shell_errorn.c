#include "../shell.h"
#include <errno.h>
#include <string.h>

/* output error message
 * ----------------------------------------------------------------------- */
int
shell_errorn(const char* s, unsigned int len) {
  buffer_put(shell_buff, s, len);
  buffer_putm_internal(shell_buff, ": ", strerror(errno), "\n", NULL);
  buffer_flush(shell_buff);
  return 1;
}
