#include "../utf8.h"

int
wctou8(char* m, wchar_t w) {
  /* Unicode Table 3-5. UTF-8 Bit Distribution
  Unicode                     1st Byte 2nd Byte 3rd Byte 4th Byte
  00000000 0xxxxxxx           0xxxxxxx
  00000yyy yyxxxxxx           110yyyyy 10xxxxxx
  zzzzyyyy yyxxxxxx           1110zzzz 10yyyyyy 10xxxxxx
  000uuuuu zzzzyyyy yyxxxxxx  11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
  */

  if(!(w & ~0x7f)) {
    m[0] = w & 0x7f;
    m[1] = '\0';
    return 1;
  } else if(!(w & ~0x7ff)) {
    m[0] = ((w >> 6) & 0x1f) | 0xc0;
    m[1] = (w & 0x3f) | 0x80;
    m[2] = '\0';
    return 2;
  } else if(!(w & ~0xffff)) {
    m[0] = ((w >> 12) & 0x0f) | 0xe0;
    m[1] = ((w >> 6) & 0x3f) | 0x80;
    m[2] = (w & 0x3f) | 0x80;
    m[3] = '\0';
    return 3;
  } else if(!(w & ~0x1fffff)) {
    m[0] = ((w >> 18) & 0x07) | 0xf0;
    m[1] = ((w >> 12) & 0x3f) | 0x80;
    m[2] = ((w >> 6) & 0x3f) | 0x80;
    m[3] = (w & 0x3f) | 0x80;
    m[4] = '\0';
    return 4;
  } else
    return -1;
}
