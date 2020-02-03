#include "../buffer.h"
#include "../stralloc.h"

int
buffer_tosa(buffer* b, stralloc* sa) {
  if(stralloc_ready(sa, 1024) == 0)
    return -1;
  b->x = sa->s;
  b->p = 0;
  b->n = 0;
  b->a = 1024;
  b->fd = 0;
  b->op = (buffer_op_proto*)stralloc_write;
  b->cookie = sa;
  b->deinit = 0;
  return 0;
}
