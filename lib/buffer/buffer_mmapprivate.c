#include <buffer.h>
#include <mmap.h>

int buffer_mmapprivate(buffer* b,const char* filename) {
  if (!(b->x=mmap_private(filename,&b->n))) return -1;
  b->p=0; b->a=b->n;
  b->fd=-1;
  b->op=buffer_dummyreadmmap;
  b->todo=(b->n ? MUNMAP : 0);
  return 0;
}
