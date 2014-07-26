#include "buffer.h"

extern ssize_t buffer_stubborn(ssize_t (*op)(),int fd,const char* buf, unsigned int len);

int buffer_flush(buffer* b) {
  register int p;
  if (!(p=b->p)) return 0; /* buffer already empty */
  b->p=0;
  return buffer_stubborn(b->op,b->fd,b->x,p);
}
