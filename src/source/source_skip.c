#include "../source.h"

/* ----------------------------------------------------------------------- */
int
source_skip(void) {
  register buffer* b = source->b;
  char c;

  if(b->p < b->n) {
    c = b->x[b->p];
#ifdef DEBUG_OUTPUT_
    debug_char("source_skip", c);
#endif
    b->p++;

    if(c == '\\') {
      if(source_peek(&c) > 0 && c == '\n')
        b->p++;
    }

    if(c == '\n') {
      source_newline();

    } else {
      source->position.column++;
      source->position.offset++;
    }

    return 1;
  }

  return 0;
}

/* ----------------------------------------------------------------------- */
int
source_skipn(int n) {
  while(n-- >= 0)
    if(!source_skip())
      break;
  return n;
}
