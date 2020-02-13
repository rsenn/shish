#include "../fmt.h"

#define tohex(c) (char)((c) >= 10 ? (c)-10 + 'a' : (c) + '0')

size_t
fmt_xlong(char* dest, unsigned long i) {
  unsigned long len, tmp;
  /* first count the number of bytes needed */
  for(len = 1, tmp = i; tmp > 15; ++len) tmp >>= 4;
  if(dest)
    for(tmp = i, dest += len;;) {
      *--dest = tohex(tmp & 15);
      if(!(tmp >>= 4)) {
        break;
      };
    }
  return len;
}
