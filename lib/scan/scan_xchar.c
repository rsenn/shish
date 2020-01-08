#include "../scan.h"

size_t
scan_xchar(const char* src, unsigned char* dest) {
  const char* tmp = src;
  unsigned char l = 0;
  unsigned char c;
  while((l >> (sizeof(l) * 8 - 4)) == 0 && (c = (unsigned char)scan_fromhex((unsigned char)*tmp)) < 16) {
    l = (unsigned char)((l << 4) + c);
    if(++tmp == src + 2)
      break;
  }
  *dest = l;
  return (size_t)(tmp - src);
}
