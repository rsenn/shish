#include "source.h"

/* ----------------------------------------------------------------------- */
void source_skip(void) {
  register buffer *b = source->b;
  unsigned char c;
  
  if(b->p < b->n) {
    c = b->x[b->p];
    
    b->p++;
    
    if(c == '\n')
      source_newline();
  }
}

