#include "term.h"

/* ----------------------------------------------------------------------- */
void term_overwritec(char c) {
  if(term_pos == term_cmdline.len)
    stralloc_catb(&term_cmdline, (const char*)&c, 1);
  else
    term_cmdline.s[term_pos] = c;

  term_pos++;
  buffer_put(term_output, (const char*)&c, 1);
  buffer_flush(term_output);
}


