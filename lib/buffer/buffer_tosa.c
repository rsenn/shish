#include "stralloc.h"
#include "buffer.h"

void buffer_tosa(buffer* b, stralloc* sa) {
  b->x = (void*)(b->p = b->n = b->a = 0);
  b->fd = (long)sa;
  b->op = (ssize_t(*)())stralloc_write;
}
