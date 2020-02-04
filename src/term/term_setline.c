#include "term.h"


/* set command line
 *
 * this assumes <s> has been malloced
 * ----------------------------------------------------------------------- */
void
term_setline(const char* s, unsigned long len) {
  unsigned int oldlen = term_cmdline.len;

  term_left(term_pos);

  /* move the pointer rather than copying the string */
  stralloc_free(&term_cmdline);

  if(s)
    stralloc_copyb(&term_cmdline, s, len);
  else
    stralloc_zero(&term_cmdline);

  if(term_cmdline.len)
    buffer_put(term_output, term_cmdline.s, term_cmdline.len);

  /* when the new line is smaller then remove trailing stuff */
  if(term_cmdline.len < oldlen) {
    buffer_putnspace(term_output, oldlen - term_cmdline.len);
    term_pos = oldlen;
    term_left(oldlen - term_cmdline.len);
  }

  term_pos = term_cmdline.len;

  buffer_flush(term_output);
}
