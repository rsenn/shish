#include "../term.h"


/* ----------------------------------------------------------------------- */
void
term_backspace(void) {
  char out[3];

  if(!term_pos)
    return;

  if(term_pos == term_cmdline.len) {
    out[0] = out[2] = '\b';
    out[1] = ' ';

    buffer_put(term_output, out, 3);
  } else {
    unsigned long len;
    len = term_cmdline.len - term_pos;
    out[0] = '\b';
    buffer_putc(term_output, out[0]);
    buffer_put(term_output, &term_cmdline.s[term_pos], len);
    buffer_putspace(term_output);
    term_pos += len + 1;
    term_left(len + 1);
  }

  term_pos--;
  stralloc_remove(&term_cmdline, term_pos, 1);
  buffer_flush(term_output);
}
