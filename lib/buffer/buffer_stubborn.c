#include <errno.h>
#include "buffer.h"

int buffer_stubborn(long (*op)(),int fd,const char* buf, unsigned long len) {
  int w;
  while (len) {
    if ((w=op(fd,buf,len))<0) {
      if (errno == EINTR) continue;
      return -1;
    };
    buf+=w;
    len-=w;
  }
  return 0;
}
