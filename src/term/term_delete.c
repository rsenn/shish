#include "term.h"

void term_delete(void)
{
  unsigned long len;

  if(term_pos == term_cmdline.len)
    return;

  len = term_cmdline.len - term_pos;
  buffer_put(term_output, &term_cmdline.s[term_pos+1], len-1);
  buffer_putspace(term_output);
  term_pos += len;
  term_left(len);

  stralloc_remove(&term_cmdline, term_pos, 1);
  buffer_flush(term_output);
}

