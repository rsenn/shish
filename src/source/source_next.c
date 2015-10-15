#include "source.h"
#include "fd.h"

/* skip current char and get peek next one
 * ----------------------------------------------------------------------- */
int source_next(char *c) {
  register buffer *b = source->b;
  int ret;

  source_skip();

  ret = b->n - b->p;
  /* no data available, try to get some */
  if(!ret)
    if((ret = buffer_feed(b)) <= 0)
      return ret;

  /* got data, peek the char */
  if(c)
    *c = b->x[b->p];

  return ret;
}

