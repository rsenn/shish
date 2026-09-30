#include "../source.h"

int source_squoted = 0;
int source_comment = 0;
int source_bs = 0;
unsigned long source_skips = 0;

/* ----------------------------------------------------------------------- */
int
source_skip(void) {
  buffer* b;
  char c;

  /* a continuation at the front is not a character to skip; used-up
     alias text is popped there too, which changes source->b */
  source_peekn(0, 0);
  b = source->b;

  if(b->p < b->n) {
    c = b->x[b->p];

#ifdef DEBUG_OUTPUT_
    debug_char("source_skip", c);
#endif

    b->p++;
    source_skips++;

    if(c == '\\' && !source_bs && !source_squoted && !source_comment) {
      source_bs = 1;

      /* the peek may pop used-up alias text: the newline then belongs to
         whatever follows the alias name, not to the buffer b was taken from */
      if(source_peek(&c) > 0 && c == '\n') {
        source->b->p++;
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
