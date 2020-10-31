#include "../source.h"

/* ----------------------------------------------------------------------- */
void
source_skip(void) {
  register buffer* b = source->b;
  unsigned char c;

  if(b->p < b->n) {
    c = b->x[b->p];
#ifdef DEBUG_OUTPUT_
    debug_char("source_skip", c);
#endif
    b->p++;

    if(c == '\n')
      source_newline();
    else
      source->pos.column++;
  }
}
