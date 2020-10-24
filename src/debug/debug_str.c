#include "../debug.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

/* output a nul-terminated string
 * ----------------------------------------------------------------------- */
void
debug_str(const char* msg, const char* s, int depth, char quote) {

  buffer_puts(buffer_2, quote ? COLOR_CYAN : COLOR_YELLOW);
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
      else if(c == '\r')
        buffer_puts(buffer_2, "\\r");
      else
        buffer_putc(buffer_2, c);
      s++;
    }
  }

  if(quote)
    buffer_putc(buffer_2, quote);
  if(quote)
    buffer_puts(buffer_2, COLOR_NONE);
  buffer_flush(buffer_2);
}
#endif /* DEBUG_OUTPUT */
