#include "../term.h"

/* ----------------------------------------------------------------------- */
void
term_erase() {
  term_escape(term_output, 2, 'K');
  term_escape(term_output, 0, 'G');
  buffer_flush(term_output);
}
