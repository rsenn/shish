#include "debug.h"

#ifdef DEBUG
#include "fd.h"

/* output a nul-terminated string
 * ----------------------------------------------------------------------- */
void
debug_str(const char* msg, const char* s, int depth) {
  if(s)
    buffer_putm(fd_err->w, COLOR_YELLOW, msg, COLOR_CYAN " = \"", s, "\"" COLOR_NONE, NULL);
  else
    buffer_putm(fd_err->w, COLOR_YELLOW, msg, COLOR_CYAN " = \"\"" COLOR_NONE, NULL);
}
#endif /* DEBUG */
