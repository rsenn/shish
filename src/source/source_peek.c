#include "../source.h"

/* gets more data from buffer (at least 1 char) and reads first char, but
 * doesn't advance buffer pointer, use source_skip() for that
 * ----------------------------------------------------------------------- */
int
source_peek(char* c) {
  return source_peekn(c, 0);
  /*buffer *b;
  int ret;
  b = source->b;

  if((ret = b->n - b->p) <= 0)
    if((ret = buffer_feed(b)) <= 0)
      return ret;
  if(c)
    *c = b->x[b->p];
  return ret;*/
}
