#include "../fmt.h"

size_t
fmt_8long(char* dest, unsigned long i) {
  size_t len, len2;
  unsigned long tmp;
  /* first count the number of bytes needed */
  for(len = 1, tmp = i; tmp > 7; ++len)
    tmp >>= 3;
  if(dest) {
    len2 = len + 1;
    dest += len;
    for(tmp = i; --len2; tmp >>= 3)
      *--dest = (tmp & 7) + '0';
  }
  return len;
}
