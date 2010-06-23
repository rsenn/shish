#include "debug.h"

#ifdef DEBUG
#include <fmt.h>
#include "fd.h"

/* output a pointer
 * ----------------------------------------------------------------------- */
void debug_ptr(const char *msg, void *ptr, int depth)
{
  char buf[FMT_XLONG];
  unsigned long n;
  debug_space(depth);
  buffer_putm(fd_err->w, COLOR_YELLOW, msg, COLOR_CYAN" = ", NULL);
  n = fmt_xlong(buf, (long)ptr);
  buffer_puts(fd_err->w, "0x");
  buffer_putnspace(fd_err->w, 8 - n);
  buffer_put(fd_err->w, buf, n);
  buffer_puts(fd_err->w, COLOR_NONE"\n");
  buffer_flush(fd_err->w);
}
#endif /* DEBUG */
