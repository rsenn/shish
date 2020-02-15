#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"
#include "../../lib/fmt.h"

/* output a pointer
 * ----------------------------------------------------------------------- */
void
debug_ptr(const char* msg, void* ptr, int depth) {
  char buf[FMT_XLONG];
  unsigned long n;
  debug_space(depth, 1);
  buffer_puts(fd_err->w, COLOR_YELLOW);
  buffer_puts(fd_err->w, msg);
  buffer_puts(fd_err->w, COLOR_CYAN " = ");
  n = fmt_xlong(buf, (unsigned long)ptr);
  buffer_puts(fd_err->w, "0x");
  buffer_putnspace(fd_err->w, 8 - n);
  buffer_put(fd_err->w, buf, n);
  buffer_puts(fd_err->w, COLOR_NONE "\n");
  buffer_flush(fd_err->w);
}
#endif /* DEBUG_OUTPUT */
