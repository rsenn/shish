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
  debug_space(depth, 0);
  buffer_puts(buffer_2, COLOR_YELLOW);
  buffer_puts(buffer_2, msg);
  buffer_puts(buffer_2, COLOR_CYAN " = ");
  n = fmt_xlong(buf, (unsigned long)ptr);
  buffer_puts(buffer_2, "0x");
  buffer_putnspace(buffer_2, 8 - n);
  buffer_put(buffer_2, buf, n);
  buffer_puts(buffer_2, COLOR_NONE "\n");
  buffer_flush(buffer_2);
}
#endif /* DEBUG_OUTPUT */
