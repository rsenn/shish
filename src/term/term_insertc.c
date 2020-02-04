#include "term.h"

extern stralloc term_cmdline;

/* ----------------------------------------------------------------------- */
void
term_insertc(char c) {
  buffer_put(term_output, &c, 1);

  if(term_pos == term_cmdline.len) {
    stralloc_catb(&term_cmdline, &c, 1);
  } else {
    unsigned long len;
    len = term_cmdline.len - term_pos;
    buffer_put(term_output, &term_cmdline.s[term_pos], len);
    stralloc_insertb(&term_cmdline, &c, term_pos, 1);
    term_pos = term_cmdline.len - 1;
    term_left(len);
    /*    debug_stralloc("cmdline", &term_cmdline);
        debug_long("pos", term_pos);*/
  }

  term_pos++;
  buffer_flush(term_output);
}
