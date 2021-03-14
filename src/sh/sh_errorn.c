#include "../fd.h"
#include "../sh.h"
#include "../source.h"
#include "../../lib/shell.h"
#include <errno.h>
#include <string.h>

/* output error message
 * ----------------------------------------------------------------------- */
int
sh_errorn(const char* s, unsigned int len) {
  buffer_puts(fd_err->w, sh_name);
  buffer_puts(fd_err->w, ": ");

  if(!(source->mode & SOURCE_IACTIVE))
    source_msg();

  if(s) {
    buffer_put(fd_err->w, s, len);
    buffer_puts(fd_err->w, ": ");
  }
  buffer_puts(fd_err->w, strerror(errno));
  buffer_putnlflush(fd_err->w);
  return 1;
}
