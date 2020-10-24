#include "../debug.h"
#include "../expand.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

/* output an allocated string
 * ----------------------------------------------------------------------- */
void
debug_stralloc(const char* msg, stralloc* s, int depth, char quote) {
  if(msg) {
    buffer_puts(buffer_2, COLOR_YELLOW);
    buffer_puts(buffer_2, msg);
    buffer_puts(buffer_2, COLOR_CYAN DEBUG_EQU COLOR_NONE);
  }

  buffer_puts(buffer_2, quote ? COLOR_CYAN : COLOR_YELLOW);
  if(quote)
    buffer_putc(buffer_2, quote);

  if(s->len && s->s) {
    const char* x = s->s;
    const char* end = s->s + s->len;

    while(x < end) {
      if(*x == '\n')
        buffer_puts(buffer_2, "\\n");
      else if(*x == '\r')
        buffer_puts(buffer_2, "\\r");
      else
        buffer_putc(buffer_2, *x);
      x++;
    }
  }
  if(quote)
    buffer_putc(buffer_2, quote);

  buffer_puts(buffer_2, COLOR_NONE);
}
#endif /* DEBUG_OUTPUT */
