#include "../buffer.h"

extern int buffer_stubborn(buffer_op_proto*, fd_t fd, const char* buf, size_t len, void* b);

int
buffer_flush(buffer* b) {
  size_t p;
  ssize_t r;
  if(!(p = b->p))
    return 0; /* buffer already empty */
  b->p = 0;
  r = buffer_stubborn(b->op, b->fd, b->x, p, b);
  if(r > 0)
    r = 0;
  return (int)r;
}
