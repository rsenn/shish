#include "buffer.h"

void buffer_init(buffer* b,long (*op)(),int fd,
		 char* y,unsigned long ylen) {
  b->op=op;
  b->fd=fd;
  b->x=y;
  b->a=ylen;
  b->p=0;
  b->n=0;
  b->todo=NOTHING;
}
