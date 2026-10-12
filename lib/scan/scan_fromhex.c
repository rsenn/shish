#include "../scan.h"

int
scan_fromhex(unsigned char c) {
  c -= '0';

  if(c <= 9)
    return c;

  c &= ~0x20;
  c -= 'A' - '0';

  if(c < 6)
    return c + 10;

  return -1;
}
