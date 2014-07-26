#include <errno.h>
#include "buffer.h"

int buffer_stubborn(int (*op)(),int fd,const unsigned char* buf, unsigned long int len) {
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
