#include "../buffer.h"
#include "../str.h"

int
buffer_putns(buffer* b, const char* s, int ntimes) {
  size_t len = str_len(s);
  while(ntimes-- > 0)
    if(buffer_put(b, s, len) < 0)
      return -1;
  return 0;
}
