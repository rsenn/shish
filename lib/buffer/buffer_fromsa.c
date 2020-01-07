#include "buffer.h"
#include "stralloc.h"

void
buffer_fromsa(buffer* b, stralloc* sa) {
  buffer_frombuf(b, sa->s, sa->len);
}
