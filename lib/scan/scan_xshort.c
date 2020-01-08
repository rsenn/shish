#include "../scan.h"

size_t
scan_xshort(const char* src, unsigned short* dest) {
  const char* tmp = src;
  unsigned short l = 0;
  unsigned char c;
  while((l >> (sizeof(l) * 8 - 4)) == 0 && (c = (unsigned char)scan_fromhex((unsigned char)*tmp)) < 16) {
    l = (unsigned short)((l << 4) + c);
    if(++tmp == src + 4)
      break;
  }
  *dest = l;
  return (size_t)(tmp - src);
}
