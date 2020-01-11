#include "../buffer.h"
#include "../mmap.h"

ssize_t buffer_dummyreadmmap(fd_t, void*, size_t, void*);

int
buffer_mmapread(buffer* b, const char* filename) {
  if(!(b->x = (char*)mmap_read(filename, &b->n)))
    return -1;
  b->p = 0;
  b->a = b->n;
  b->fd = -1;
  b->op = &buffer_dummyreadmmap;
  b->deinit = buffer_munmap;
  return 0;
}
