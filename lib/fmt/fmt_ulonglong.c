#include "../fmt.h"
#include "../uint64.h"

size_t
fmt_ulonglong(char* dest, uint64 i) {
  size_t len, len2;
  uint64 tmp = i;
  /* first count the number of bytes needed */
  for(len = 1; tmp > 9; ++len)
    tmp /= 10;

  if(dest) {
    dest += len;
    len2 = len + 1;

    for(tmp = i; --len2; tmp /= 10)
      *--dest = (tmp % 10) + '0';
  }
  return len;
}
