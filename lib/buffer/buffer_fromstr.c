#include "str.h"
#include "buffer.h"

void buffer_fromstr(buffer *b, char *s, unsigned long len)
{
  b->x=s;
  b->p=0;
  b->n=len;
  b->a=b->n+1;
  b->fd=-1;
  b->op=buffer_dummyread;
}
