#include "debug.h"

#ifdef DEBUG
#include "fd.h"

/* output an unsigned long
 * ----------------------------------------------------------------------- */
void
debug_ulong(const char* msg, unsigned long l, int depth) {

  buffer_puts(fd_err->w, COLOR_YELLOW);
  buffer_puts(fd_err->w, msg);
  buffer_puts(fd_err->w, COLOR_CYAN DEBUG_EQU);
  buffer_putulong(fd_err->w, l);
  buffer_puts(fd_err->w, COLOR_NONE);
}
#endif /* DEBUG */
