#include "../buffer.h"
#include "../mmap.h"

ssize_t buffer_dummyreadmmap(int, void*, size_t, void*);
void buffer_munmap(buffer* buf);

int
buffer_mmapread_fd(buffer* b, int fd) {
  if(!(b->x = mmap_read_fd(fd, &b->n)))
    return -1;
  b->p = 0;
  b->a = b->n;
  b->fd = fd;
  b->op = (buffer_op_proto*)&buffer_dummyreadmmap;
  if(b->n)
    b->deinit = (void (*)()) & buffer_munmap; /*    b->todo=MUNMAP; */
  return 0;
}
