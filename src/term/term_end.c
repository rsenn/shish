#include "term.h"


/* ----------------------------------------------------------------------- */
void
term_end(void) {
  if(term_pos == term_cmdline.len)
    return;

  if(term_cmdline.len - term_pos > 4 && !term_dumb)
    term_escape(term_cmdline.len - term_pos, 'C');
  else
    buffer_put(term_output, &term_cmdline.s[term_pos], term_cmdline.len - term_pos);

  buffer_flush(term_output);
  term_pos = term_cmdline.len;
}
