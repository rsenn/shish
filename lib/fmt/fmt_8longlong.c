#include "../uint64.h"
#include "../fmt.h"

size_t
fmt_8longlong(char* dest, uint64 i) {
  size_t len, len2;
  uint64 tmp;
  /* first count the number of bytes needed */
  for(len = 1, tmp = i; tmp > 7; ++len) tmp >>= 3;
  if(dest) {
    len2 = len + 1;
    dest += len;
    for(tmp = i; --len2; tmp >>= 3) *--dest = (tmp & 7) + '0';
  }
  return len;
}
