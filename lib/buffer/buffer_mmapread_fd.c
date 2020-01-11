#include "../buffer.h"
#include "../mmap.h"

extern ssize_t buffer_dummyreadmmap();
extern void buffer_munmap(void* buf);

int
buffer_mmapread_fd(buffer* b, fd_t fd) {
  if(!(b->x = mmap_read_fd(fd, &b->n)))
    return -1;
  b->p = 0;
  b->a = b->n;
  b->fd = fd;
  b->op = buffer_dummyreadmmap;
  if(b->n)
    b->deinit = buffer_munmap; /*    b->todo=MUNMAP; */
  return 0;
}
