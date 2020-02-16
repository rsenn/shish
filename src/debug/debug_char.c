#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

/* output a char
 * ----------------------------------------------------------------------- */
void
debug_char(const char* msg, char c, int depth) {
  debug_space(depth, 0);
  buffer_puts(fd_err->w, msg);
  buffer_puts(fd_err->w, " = ");
  buffer_put(fd_err->w, &c, 1);
  buffer_putnlflush(fd_err->w);
}
#endif /* defined(DEBUG_OUTPUT) */