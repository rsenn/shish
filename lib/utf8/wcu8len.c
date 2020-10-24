#include "../utf8.h"

int
wcu8len(const wchar_t w) {
  if(!(w & ~0x7f))
    return 1;
  if(!(w & ~0x7ff))
    return 2;
  if(!(w & ~0xffff))
    return 3;
  if(!(w & ~0x1fffff))
    return 4;
  return -1; /* error */
}
