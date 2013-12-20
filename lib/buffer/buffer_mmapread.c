#include <buffer.h>
#include <mmap.h>

ssize_t buffer_dummyread() {
  return 0;
}

int buffer_mmapread(buffer* b,const char* filename) {
  if (!(b->x=mmap_read(filename,&b->n))) return -1;
  b->p=0; b->a=b->n;
  b->fd=-1;
  b->op=buffer_dummyread;
  b->todo=MUNMAP;
  return 0;
}
