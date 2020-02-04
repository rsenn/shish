#include "term.h"


/* ----------------------------------------------------------------------- */
void
term_right(unsigned long n) {
  if(term_pos == term_cmdline.len || !n)
    return;

  if(n + term_pos > term_cmdline.len)
    n = term_cmdline.len - term_pos;

  buffer_put(term_output, &term_cmdline.s[term_pos], n);
  buffer_flush(term_output);
  term_pos += n;
}
