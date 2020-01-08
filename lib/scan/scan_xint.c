#include "../scan.h"

size_t
scan_xint(const char* src, unsigned int* dest) {
  const char* tmp = src;
  unsigned int l = 0;
  unsigned char c;
  while((l >> (sizeof(l) * 8 - 4)) == 0 && (c = (unsigned char)scan_fromhex((unsigned char)*tmp)) < 16) {
    l = (l << 4) + c;
    ++tmp;
  }
  *dest = l;
  return (size_t)(tmp - src);
}
