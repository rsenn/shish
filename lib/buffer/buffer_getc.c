#include "../buffer.h"
#include "../byte.h"

ssize_t
buffer_getc(buffer* b, char* x) {
  if(b->p == b->n) {
    ssize_t blen;
    if((blen = buffer_feed(b)) <= 0)
      return blen;
  }
  *x = b->x[b->p];
  ++b->p;
  return 1;
}
