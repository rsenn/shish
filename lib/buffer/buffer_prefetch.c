#include <byte.h>
#include <buffer.h>

extern int buffer_stubborn_read(int (*op)(),int fd,const char* buf, unsigned int len);

int buffer_prefetch(buffer* b, unsigned long n) {
  if(b->p && b->p+n>=b->a)
  {
    if(b->op == buffer_dummyread)
      return (b->n-b->p);
    
    byte_copy(b->x, b->n - b->p, &b->x[b->p]);
    b->n -= b->p;
    b->p = 0;
  }
  if(b->p+n>=b->a)
    n = b->a - b->p;
  
  if(n == 0)
    return -1;
  
  while (b->n<b->p+n) {
    int w;
    if ((w=buffer_stubborn_read(b->op,b->fd,&b->x[b->n],b->a-b->n))<0)
      return -1;
    b->n+=w;
    if(!w) break;
  }
  return (b->n-b->p);
}
