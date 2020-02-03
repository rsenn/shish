#include "../windoze.h"
#include "../buffer.h"
#include <errno.h>

#if !WINDOWS_NATIVE
#include <unistd.h>
#endif

#ifndef EINTR
#define EINTR 4
#endif

int
buffer_stubborn(buffer_op_proto* op, fd_t fd, const char* buf, size_t len, void* b) {
  ssize_t w;
  errno = 0;
  while(len) {
    if((w = ((buffer_op_proto*)op)(fd, (void*)buf, len, b)) <= 0) {
      if(errno == EINTR)
        continue;
      return -1;
    }
    buf += w;
    len -= (size_t)w;
  }
  return 0;
}
