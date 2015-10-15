#include "source.h"

/* gets more data from buffer (at least n + 1 chars)
 * doesn't advance buffer pointer, use input_skipcn() for that
 * ----------------------------------------------------------------------- */
int source_peekn(char *c, unsigned long n) {
  register buffer *b = source->b;
  int ret = b->n - b->p;

  /* no data available, try to get some */
  if(ret <= n)
    if((ret = buffer_prefetch(b, n + 1)) <= 0)
      return ret;

  /* got data, peek the char */
  if(c)
    *c = b->x[b->p + n];

  return ret;
}
