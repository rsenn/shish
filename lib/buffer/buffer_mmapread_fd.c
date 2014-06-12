#include "buffer.h"
#include "mmap.h"

extern ssize_t buffer_dummyreadmmap();

int buffer_mmapread_fd(buffer *b, int fd) 
{
  if (!(b->x=mmap_read_fd(fd, &b->n))) return -1;
  b->p=0; b->a=b->n;
  b->fd=fd;
  b->op=buffer_dummyreadmmap;
  if(b->n)
    b->todo=MUNMAP;
  return 0;
}
