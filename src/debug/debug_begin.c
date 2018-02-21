#include "debug.h"

#ifdef DEBUG
#include "fd.h"

/* begin a {}-block
 * ----------------------------------------------------------------------- */
void debug_begin(const char *s, int depth)
{
  debug_space(depth);
  buffer_putm(fd_err->w, COLOR_YELLOW, s, COLOR_CYAN, " ", /* DEBUG_EQU, */
                             DEBUG_BEGIN, COLOR_NONE, NULL);
}
#endif /* DEBUG */
