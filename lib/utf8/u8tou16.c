#include "../utf8.h"

/* UTF-8 to UTF-16 (Windows wide strings), only whole characters, at most max
 * units, no terminator; returns the units written. An invalid or cut-off byte
 * becomes U+FFFD. *used (may be NULL) gets the number of bytes consumed. */
size_t
u8tou16(unsigned short* dst, size_t max, const char* s, size_t n, size_t* used) {
  size_t i = 0, out = 0;

  while(i < n) {
    unsigned cp;
    int l = u8decode(s + i, n - i, &cp);
    size_t units;

    if(l < 0) {
      cp = 0xfffd;
      l = 1;
    }

    units = cp >= 0x10000 ? 2 : 1;

    if(out + units > max)
      break;

    if(units == 2) {
      cp -= 0x10000;
      dst[out++] = (unsigned short)(0xd800 + (cp >> 10));
      dst[out++] = (unsigned short)(0xdc00 + (cp & 0x3ff));
    } else {
      dst[out++] = (unsigned short)cp;
    }

    i += (size_t)l;
  }

  if(used)
    *used = i;

  return out;
}
