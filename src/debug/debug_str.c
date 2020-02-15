#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

/* output a nul-terminated string
 * ----------------------------------------------------------------------- */
void
debug_str(const char* msg, const char* s, int depth, char quote) {
  buffer_puts(fd_err->w, COLOR_YELLOW);
  if(msg) {
    buffer_puts(fd_err->w, msg);
    buffer_puts(fd_err->w, COLOR_CYAN "=");
  }
  if(quote)
    buffer_putc(fd_err->w, quote);

  if(s)
    buffer_puts(fd_err->w, s);
  if(quote)
    buffer_putc(fd_err->w, quote);
  buffer_puts(fd_err->w, COLOR_NONE);
}
#endif /* DEBUG_OUTPUT */
