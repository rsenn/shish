#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

/* begin a {}-block
 * ----------------------------------------------------------------------- */
void
debug_begin(const char* s, int depth) {
  debug_space(depth - 1, 0);
  debug_space(depth, 0);
  buffer_puts(buffer_2, COLOR_YELLOW);
  buffer_puts(buffer_2, s);
  buffer_puts(buffer_2, COLOR_CYAN DEBUG_EQU DEBUG_BEGIN COLOR_NONE);
}
#endif /* DEBUG_OUTPUT */
