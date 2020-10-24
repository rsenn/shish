#include "../term.h"

void
term_newline(void) {
  char out = '\n';
  stralloc_catb(&term_cmdline, &out, 1);
  buffer_put(term_output, &out, 1);
  buffer_flush(term_output);
  term_pos = 0;

#ifdef DEBUG_OUTPUT
  buffer_putsflush(buffer_2, "term_newline\n");
#endif
}
