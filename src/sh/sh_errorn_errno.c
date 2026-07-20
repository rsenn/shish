#include "../fd.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../source.h"
#include "../../lib/shell.h"
#include <errno.h>
#include <string.h>

/* like sh_errorn(), but for reporting a syscall that just failed:
 * appends ": <strerror(errno)>" to the message.
 * ----------------------------------------------------------------------- */
int
sh_errorn_errno(const char* s, unsigned int len) {
  sh_msgn(s, len);

  if(errno) {
    if(s)
      buffer_puts(fd_err->w, ": ");
    buffer_puts(fd_err->w, strerror(errno));
  }

  buffer_putnlflush(fd_err->w);
  return 1;
}
