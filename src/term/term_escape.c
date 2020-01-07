#include "term.h"

/* put an ANSI escape sequence to the terminal
 * ----------------------------------------------------------------------- */
void
term_escape(unsigned long n, char type) {
  buffer_puts(term_output, "\033[");
  if(n > 1)
    buffer_putulong(term_output, n);
  buffer_put(term_output, &type, 1);
}
