#include "../buffer.h"
#include "../stralloc.h"

int
buffer_putsa(buffer* b, const stralloc* sa) {
  return buffer_put(b, sa->s, sa->len);
}
