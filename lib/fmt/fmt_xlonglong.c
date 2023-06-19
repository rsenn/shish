#include "../fmt.h"

#define tohex(c) (char)((c) >= 10 ? (c)-10 + 'a' : (c) + '0')

size_t
fmt_xlonglong(char* dest, uint64 i) {
  size_t len, len2;
  uint64 tmp;
  /* first count the number of bytes needed */
  for(len = 1, tmp = i; tmp > 15; ++len)
    tmp >>= 4;
  if(dest) {
    len2 = len + 1;
    dest += len;
    for(tmp = i; --len2; tmp >>= 4)
      *--dest = tohex(tmp & 15);
  }
  return len;
}
