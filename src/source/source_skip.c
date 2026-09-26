#include "../source.h"

int source_squoted = 0;
int source_comment = 0;
int source_bs = 0;

/* ----------------------------------------------------------------------- */
int
source_skip(void) {
  buffer* b = source->b;
  char c;

  /* a continuation at the front is not a character to skip */
  source_peekn(0, 0);

  if(b->p < b->n) {
    c = b->x[b->p];

#ifdef DEBUG_OUTPUT_
    debug_char("source_skip", c);
#endif

    b->p++;

    if(c == '\\' && !source_bs && !source_squoted && !source_comment) {
      source_bs = 1;

      if(source_peek(&c) > 0 && c == '\n') {
        b->p++;
        source_bs = 0;
      } else {
        c = '\\';
      }
    } else {
      source_bs = 0;
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
