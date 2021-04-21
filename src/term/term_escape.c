#include "../../lib/buffer.h"

/* put an ANSI escape sequence to the terminal
 * ----------------------------------------------------------------------- */
void
term_escape(buffer* b, long n, char type) {
  buffer_puts(b, "\033[");
  if(n >= 0)
    buffer_putulong(b, n);
  buffer_put(b, &type, 1);
}
