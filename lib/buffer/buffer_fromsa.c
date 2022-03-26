#include "../buffer.h"
#include "../stralloc.h"

void
buffer_fromsa(buffer* b, const stralloc* sa) {
  buffer_frombuf(b, sa->s, sa->len);
}
