#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

/* end a {}-block
 * ----------------------------------------------------------------------- */
void
debug_end(int depth) {

  if(depth > 0) {
    debug_space(depth - 1, 0);
  }
  buffer_puts(buffer_2, COLOR_CYAN DEBUG_END COLOR_NONE);
}
#endif /* DEBUG_OUTPUT */
