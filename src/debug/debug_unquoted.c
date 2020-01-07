#include "debug.h"

#ifdef DEBUG
#include "fd.h"

/* output a nul-terminated string
 * ----------------------------------------------------------------------- */
void
debug_unquoted(const char* msg, const char* s, int depth) {
  if(msg)
    buffer_putm(fd_err->w, COLOR_YELLOW, msg, COLOR_CYAN, DEBUG_EQU, COLOR_RED, s, COLOR_NONE, NULL);
  else
    buffer_putm(fd_err->w, COLOR_RED, s, COLOR_NONE, NULL);
}
#endif /* DEBUG */
