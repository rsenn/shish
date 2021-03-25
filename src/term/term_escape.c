#include "../term.h"

/* put an ANSI escape sequence to the terminal
 * ----------------------------------------------------------------------- */
void
term_escape(buffer* b, unsigned long n, char type) {
  buffer_puts(b, "\033[");
  if(n > 1)
    buffer_putulong(b, n);
  buffer_put(b, &type, 1);
}
