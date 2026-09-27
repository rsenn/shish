#include "../utf8.h"

/* the code point at s[0..n), strictly: no overlong forms, no surrogates,
 * nothing above U+10FFFF (the shape of QuickJS's unicode_from_utf8(), without
 * its 5/6-byte forms and with truncation told apart from invalid input) */
int
u8decode(const char* s, size_t n, unsigned* cp) {
  const unsigned char* p = (const unsigned char*)s;
  unsigned c, min;
  int len, i;

  if(n == 0)
    return 0;

  c = p[0];

  if(c < 0x80) {
    *cp = c;
    return 1;
  } else if(c >= 0xc2 && c <= 0xdf) {
    len = 2, c &= 0x1f, min = 0x80;
  } else if((c & 0xf0) == 0xe0) {
    len = 3, c &= 0x0f, min = 0x800;
  } else if(c >= 0xf0 && c <= 0xf4) {
    len = 4, c &= 0x07, min = 0x10000;
  } else {
    return -1;
  }

  for(i = 1; i < len; i++) {
    if((size_t)i >= n)
      return -2;

    if((p[i] & 0xc0) != 0x80)
      return -1;

    c = (c << 6) | (p[i] & 0x3f);
  }

  if(c < min || c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff))
    return -1;

  *cp = c;
  return len;
}
