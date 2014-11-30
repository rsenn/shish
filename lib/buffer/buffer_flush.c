#include "buffer.h"

extern long buffer_stubborn(long (*op)(),int fd,const char* buf, unsigned int len);

int buffer_flush(buffer* b) {
  register int p;
  if (!(p=b->p)) return 0; /* buffer already empty */
  b->p=0;
  return buffer_stubborn(b->op,b->fd,b->x,p);
}
