#include "../scan.h"

size_t
scan_xlonglong(const char* src, uint64* dest) {
  const char* tmp = src;
  int64 l = 0;
  unsigned char c;
  while((c = scan_fromhex(*tmp)) < 16) {
    l = (l << 4) + c;
    ++tmp;
  }
  *dest = l;
  return tmp - src;
}
