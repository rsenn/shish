#include "debug.h"

#ifdef DEBUG
#include "fd.h"

/* end a {}-block
 * ----------------------------------------------------------------------- */
void
debug_end(int depth) {
  buffer_puts(fd_err->w, COLOR_CYAN DEBUG_END COLOR_NONE);
}
#endif /* DEBUG */
