#include "../windoze.h"
#include "../buffer.h"
#include <errno.h>

#if !WINDOWS_NATIVE
#include <unistd.h>
#endif

#ifndef EINTR
#define EINTR 4
#endif

ssize_t
buffer_stubborn_read(buffer_op_proto* op, fd_t fd, const void* buf, size_t len, void* ptr) {
  ssize_t w;
  for(;;) {
    if((w = (*op)(fd, (void*)buf, len, ptr)) < 0)
      if(errno == EINTR)
        continue;
    break;
  }
  return w;
}
