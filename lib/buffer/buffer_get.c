#include "byte.h"
#include "buffer.h"

int
buffer_get(buffer* b, char* x, size_t len) {
  ssize_t blen;
  if((blen =
buffer_feed(b)) <= 0) return blen;
  if((size_t)  blen >= len)
    blen = len;
  byte_copy(x, blen, b->x + b->p);
  b->p += blen;
  return blen;
}
