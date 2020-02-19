#include "../buffer.h"

int
buffer_putnc(buffer* b, char c, int ntimes) {
  while(ntimes-- > 0)
    if(buffer_put(b, &c, 1) < 0)
      return -1;
  return 0;
}
