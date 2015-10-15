#include "term.h"

/* ----------------------------------------------------------------------- */
void term_left(unsigned long n) {
  if(!term_pos || !n)
    return;
  
  if(n > term_pos)
    n = term_pos;
      
  if(n > 4 && !term_dumb) {
    term_escape(n, 'D');
  } else {
    int i;
    char c = '\b';
      
    for(i = 0; i < n; i++)
      buffer_PUTC(term_output, c);
  }
      
  buffer_flush(term_output);
  term_pos -= n;
}


