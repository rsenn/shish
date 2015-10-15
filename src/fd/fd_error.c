#include "fmt.h"
#include "str.h"
#include "sh.h"
#include "fd.h"

/* print fd number + error message
 * ----------------------------------------------------------------------- */
int fd_error(int n, const char *msg) {
  char buf[FMT_ULONG + 2];
  unsigned long sz;
  sz = fmt_ulong(buf, n);
  str_copy(&buf[sz], ": ");
  sh_msg(buf);
  buffer_puts(fd_err->w, msg);
  buffer_putnlflush(fd_err->w);
  return -1;
}

