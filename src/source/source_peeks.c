#include "../source.h"

/* ----------------------------------------------------------------------- */
char*
source_peeks(unsigned n) {
  buffer* b = source->b;
  int ret = b->n - b->p;

  /* no data available, try to get some */
  if((unsigned)ret <= n)
    if((ret = buffer_prefetch(b, n + 1)) <= 0)
      return 0;
  return &b->x[b->p + n];
}
