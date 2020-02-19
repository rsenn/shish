#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

/* output a nul-terminated string
 * ----------------------------------------------------------------------- */
void
debug_str(const char* msg, const char* s, int depth, char quote) {
  if(quote)
    buffer_puts(buffer_2, COLOR_CYAN);
  if(msg) {
    buffer_puts(buffer_2, msg);
    buffer_puts(buffer_2, COLOR_CYAN "=");
  }
  if(quote)
    buffer_putc(buffer_2, quote);

  if(s) {
    while(*s) {
      char c = *s;
      if(c == '\n')
        buffer_puts(buffer_2, "\\n");
      else
        buffer_putc(buffer_2, c);
      s++;
    }
  }

  if(quote)
    buffer_putc(buffer_2, quote);
  if(quote)
    buffer_puts(buffer_2, COLOR_NONE);
}
#endif /* DEBUG_OUTPUT */
