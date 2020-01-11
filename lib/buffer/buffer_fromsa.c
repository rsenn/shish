#include "../buffer.h"

typedef struct stralloc_s {
  char* s;
  size_t len;
  size_t a;
} stralloc;

void
buffer_fromsa(buffer* b, const stralloc* sa) {
  buffer_frombuf(b, sa->s, sa->len);
}
