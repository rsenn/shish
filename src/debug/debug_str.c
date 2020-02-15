#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

/* output a nul-terminated string
 * ----------------------------------------------------------------------- */
void
debug_str(const char* msg, const char* s, int depth, char quote) {
  if(quote)
    buffer_puts(fd_err->w, COLOR_CYAN);
  if(msg) {
    buffer_puts(fd_err->w, msg);
    buffer_puts(fd_err->w, COLOR_CYAN "=");
  }
  if(quote)
    buffer_putc(fd_err->w, quote);

  if(s) {
    while(*s) {
      char c = *s;
      if(c == '\n')
        buffer_puts(fd_err->w, "\\n");
      else
        buffer_putc(fd_err->w, c);
      s++;
    }
  }

  if(quote)
    buffer_putc(fd_err->w, quote);
  if(quote)
    buffer_puts(fd_err->w, COLOR_NONE);
}
#endif /* DEBUG_OUTPUT */
