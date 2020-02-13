#include "../buffer.h"
#include "../byte.h"
#include <string.h>

extern int
buffer_stubborn(buffer_op_proto* op, fd_t fd, const char* buf, size_t len, void* b);

#ifdef __dietlibc__
#undef __unlikely
#endif

#ifndef __unlikely
#define __unlikely(x) (x)
#endif

int
buffer_put(buffer* b, const char* buf, size_t len) {
  if(__unlikely(len > b->a - b->p)) { /* doesn't fit */
    if(buffer_flush(b) == -1)
      return -1;
    if(len > b->a) {
      if(buffer_stubborn(b->op, b->fd, buf, len, b) < 0)
        return -1;
      return 0;
    }
  }
  byte_copy(b->x + b->p, len, buf);
  b->p += len;
  return 0;
}
