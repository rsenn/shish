#include "../utf8.h"

/* writes the UTF-8 form of cp (no terminator); 0 for a surrogate or a value
 * above U+10FFFF */
int
u8encode(char* dst, unsigned cp) {
  if(cp < 0x80) {
    dst[0] = (char)cp;
    return 1;
  }

  if(cp < 0x800) {
    dst[0] = (char)(0xc0 | (cp >> 6));
    dst[1] = (char)(0x80 | (cp & 0x3f));
    return 2;
  }

  if(cp >= 0xd800 && cp <= 0xdfff)
    return 0;

  if(cp < 0x10000) {
    dst[0] = (char)(0xe0 | (cp >> 12));
    dst[1] = (char)(0x80 | ((cp >> 6) & 0x3f));
    dst[2] = (char)(0x80 | (cp & 0x3f));
    return 3;
  }

  if(cp > 0x10ffff)
    return 0;

  dst[0] = (char)(0xf0 | (cp >> 18));
  dst[1] = (char)(0x80 | ((cp >> 12) & 0x3f));
  dst[2] = (char)(0x80 | ((cp >> 6) & 0x3f));
  dst[3] = (char)(0x80 | (cp & 0x3f));
  return 4;
}
