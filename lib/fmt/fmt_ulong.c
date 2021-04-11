#include "../fmt.h"

size_t
fmt_ulong(char* dest, unsigned long i) {
  size_t len, len2;
  unsigned long tmp = i;
  /* first count the number of bytes needed */
  for(len = 1; tmp > 9; ++len) tmp /= 10;
  if(dest) {
    dest += len;
    len2 = len + 1;
    for(tmp = i; --len2; tmp /= 10) *--dest = (tmp % 10) + '0';
  }
  return len;
}
