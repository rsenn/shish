#include "../buffer.h"

int
buffer_putns(buffer* b, const char* s, int ntimes) {
  while(ntimes-- > 0)
    if(buffer_puts(b, s) < 0)
      return -1;
  return 0;
}
