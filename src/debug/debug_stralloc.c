#include "../debug.h"
#include "../expand.h"

#ifdef DEBUG_OUTPUT
#include "../fd.h"

/* output an allocated string
 * ----------------------------------------------------------------------- */
void
debug_stralloc(const char* msg, stralloc* s, int depth, char quote) {
  if(msg) {
    buffer_puts(fd_err->w, COLOR_YELLOW);
    buffer_puts(fd_err->w, msg);
    buffer_puts(fd_err->w, COLOR_CYAN DEBUG_EQU COLOR_NONE);
  }

  buffer_puts(fd_err->w, quote ? COLOR_CYAN : COLOR_YELLOW);
  if(quote)
    buffer_putc(fd_err->w, quote);

  if(s->len && s->s) {
    const char* x = s->s;
    const char* end = s->s + s->len;

    while(x < end) {
      /*  if(*x == '\n')
          buffer_puts(fd_err->w, "\\n");
        else*/

      buffer_putc(fd_err->w, *x);
      x++;
    }
  }
  if(quote)
    buffer_putc(fd_err->w, quote);

  buffer_puts(fd_err->w, COLOR_NONE);
}
#endif /* DEBUG_OUTPUT */
