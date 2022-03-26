#include "../buffer.h"

ssize_t
buffer_dummyreadbuf(fd_t fd, void* buf, size_t len, void* arg) {
  (void)fd;
  (void)buf;
  (void)len;
  return 0;
}

void
buffer_frombuf(buffer* b, const char* x, size_t l) {
  b->x = (char*)x;
  b->p = 0;
  b->n = l;
  b->a = l;
  b->fd = 0;
  b->op = &buffer_dummyreadbuf;
  b->deinit = 0;
}
