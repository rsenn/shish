#include "../fd.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../source.h"
#include "../../lib/shell.h"
#include <errno.h>
#include <string.h>

/* output error message
 * ----------------------------------------------------------------------- */
int
sh_errorn(const char* s, unsigned int len) {
  sh_msgn(s, len);

  if(errno) {
    if(s)
      buffer_puts(fd_err->w, ": ");
    buffer_puts(fd_err->w, strerror(errno));
  }
  buffer_putnlflush(fd_err->w);
  return 1;
}
