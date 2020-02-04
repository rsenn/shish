#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

/* begin a {}-block
 * ----------------------------------------------------------------------- */
void
debug_begin(const char* s, int depth) {
  debug_space(depth - 1, 1);
  debug_space(depth, 1);
  buffer_puts(fd_err->w, COLOR_YELLOW);
  buffer_puts(fd_err->w, s);
  buffer_puts(fd_err->w, COLOR_CYAN DEBUG_EQU DEBUG_BEGIN COLOR_NONE);
}
#endif /* DEBUG_OUTPUT */
