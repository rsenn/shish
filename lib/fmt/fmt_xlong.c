#include "../fmt.h"

#define tohex(c) (char)((c) >= 10 ? (c)-10 + 'a' : (c) + '0')

size_t
fmt_xlong(char* dest, unsigned long i) {
  size_t len, len2;
  unsigned long tmp;
  /* first count the number of bytes needed */
  for(len = 1, tmp = i; tmp > 15; ++len) tmp >>= 4;
  if(dest) {
    len2 = len + 1;
    dest += len;
    for(tmp = i; --len2; tmp >>= 4) *--dest = tohex(tmp & 15);
  }
  return len;
}
