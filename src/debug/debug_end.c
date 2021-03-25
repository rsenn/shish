#define DEBUG_NOCOLOR 1
#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

/* end a {}-block
 * ----------------------------------------------------------------------- */
void
debug_end(int depth) {
  debug_newline(depth);

  debug_s(COLOR_CYAN DEBUG_END COLOR_NONE);
  debug_fl();
}
#endif /* DEBUG_OUTPUT */
