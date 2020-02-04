#include "../debug.h"

#ifdef DEBUG
#include "../fd.h"

/* output a nul-terminated string
 * ----------------------------------------------------------------------- */
void
debug_unquoted(const char* msg, const char* s, int depth) {

  if(msg) {
    buffer_puts(fd_err->w, COLOR_YELLOW);
    buffer_puts(fd_err->w, msg);
    buffer_puts(fd_err->w, COLOR_CYAN DEBUG_EQU);
  }
  buffer_puts(fd_err->w, COLOR_RED);
  buffer_puts(fd_err->w, s);
  buffer_puts(fd_err->w, COLOR_NONE);
}
#endif /* DEBUG */
