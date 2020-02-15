#include "../buffer.h"

int
buffer_putc(buffer* b, char c) {
  if(b->a == b->p) { /* doesn't fit */
    if(buffer_flush(b) == -1)
      return -1;
    if(b->a == 0) {
      if(buffer_stubborn(b->op, b->fd, &c, 1, b) < 0)
        return -1;
      return 0;
    }
  }
  b->x[b->p++] = c;
  return 0;
}
