#include "../utf8.h"

/* UTF-16 (Windows wide strings) to UTF-8, only whole characters, at most max
 * bytes, no terminator; returns the bytes written. A lone surrogate becomes
 * U+FFFD. *used (may be NULL) gets the number of UTF-16 units consumed. */
size_t
u8fromu16(char* dst, size_t max, const unsigned short* w, size_t n, size_t* used) {
  size_t i = 0, out = 0;

  while(i < n) {
    unsigned cp = w[i];
    size_t units = 1;
    char tmp[4];
    int l;

    if(cp >= 0xd800 && cp <= 0xdbff && i + 1 < n && w[i + 1] >= 0xdc00 && w[i + 1] <= 0xdfff) {
      cp = 0x10000 + ((cp - 0xd800) << 10) + (w[i + 1] - 0xdc00);
      units = 2;
    } else if(cp >= 0xd800 && cp <= 0xdfff) {
      cp = 0xfffd;
    }

    l = u8encode(tmp, cp);

    if(out + (size_t)l > max)
      break;

    for(int k = 0; k < l; k++)
      dst[out + (size_t)k] = tmp[k];

    out += (size_t)l;
    i += units;
  }

  if(used)
    *used = i;

  return out;
}
