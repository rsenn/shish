#include "debug.h"

#ifdef DEBUG
#include "fd.h"

/* output an allocated string
 * ----------------------------------------------------------------------- */
void debug_stralloc(const char *msg, stralloc *s, int depth)
{
  buffer_putm(fd_err->w, COLOR_YELLOW, msg, COLOR_CYAN, DEBUG_EQU, "\"", NULL);
  buffer_putsa(fd_err->w, s);
  buffer_putm(fd_err->w, "\"", COLOR_NONE, NULL);
}
#endif /* DEBUG */
