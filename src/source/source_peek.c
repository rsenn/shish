#include "../source.h"

/* gets more data from buffer (at least 1 char) and reads first char, but
 * doesn't advance buffer pointer, use source_skip() for that
 * ----------------------------------------------------------------------- */
int
source_peek(char* c) {
  register buffer* b = source->b;
  int ret = b->n - b->p;

  /* no data available, try to get some */
  if(!ret)
    if((ret = buffer_feed(b)) <= 0)
      return ret;

  /* got data, peek the char */
  if(c)
    *c = b->x[b->p];

  return ret;
}
