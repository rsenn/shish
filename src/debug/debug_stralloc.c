#include "debug.h"

#ifdef DEBUG
#include "fd.h"

/* output an allocated string
 * ----------------------------------------------------------------------- */
void
debug_stralloc(const char* msg, stralloc* s, int depth) {
    buffer_puts(fd_err->w, COLOR_YELLOW);
  buffer_puts(fd_err->w, msg);
  buffer_puts(fd_err->w, COLOR_CYAN DEBUG_EQU "\"");
  buffer_putsa(fd_err->w, s);
  buffer_puts(fd_err->w, "\""  COLOR_NONE);
}
#endif /* DEBUG */
