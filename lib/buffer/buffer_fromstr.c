#include "../buffer.h"
#include "../str.h"

ssize_t
buffer_dummyread_fromstr() {
  return 0;
}

void
buffer_fromstr(buffer* b, char* s, size_t len) {
  b->x = s;
  b->p = 0;
  b->n = len;
  b->a = b->n + 1;
  b->fd = -1;
  b->op = (void*)&buffer_dummyread_fromstr;
}
