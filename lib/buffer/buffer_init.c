#include "../buffer.h"

void
buffer_init(buffer* b, buffer_op_proto* op, int fd, char* y, size_t ylen) {
  b->op = (buffer_op_proto*)(void*)op;
  b->fd = fd;
  b->x = y;
  b->a = ylen;
  b->p = 0;
  b->n = 0;
  b->cookie = 0;
  b->deinit = 0;
}
