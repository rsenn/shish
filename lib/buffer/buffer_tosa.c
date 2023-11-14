#include "../buffer.h"
#include "../stralloc.h"

static ssize_t
strallocwrite(fd_t fd, char* buf, size_t len, void* myself) {
  buffer* b = myself;
  stralloc* sa = b->cookie;
  sa->len += len;

  if(stralloc_readyplus(sa, 1024) == 0)
    return 0;
  b->x = sa->s + sa->len;
  b->p = 0;
  b->a = 1024;
  (void)fd;
  (void)buf;
  return (ssize_t)len;
}

int
buffer_tosa(buffer* b, stralloc* sa) {
  if(stralloc_ready(sa, 1024) == 0)
    return -1;
  b->x = sa->s;
  b->p = 0;
  b->n = 0;
  b->a = 1024;
  b->fd = 0;
  b->op = (buffer_op_fn*)strallocwrite;
  b->cookie = sa;
  b->deinit = 0;
  return 0;
}
