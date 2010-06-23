#include <buffer.h>
#include <mmap.h>

int buffer_dummyreadmmap()
{
  return 0;
}

int buffer_mmapread(buffer* b,const char* filename) {
  if (!(b->x=mmap_read(filename,&b->n))) return -1;
  b->p=0; b->a=b->n;
  b->fd=-1;
  b->op=buffer_dummyreadmmap;
  b->todo=MUNMAP;
  return 0;
}
