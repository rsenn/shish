#include "../scan.h"

size_t
scan_ulonglong(const char* src, uint64* dest) {
  const char* tmp = src;
  uint64 l = 0;
  unsigned char c;
  while((c = (unsigned char)(*tmp - '0')) < 10) {
    uint64 n;
    /* division is very slow on most architectures */
    n = l << 3;
    if((n >> 3) != l)
      break;
    if(n + (l << 1) < n)
      break;
    n += l << 1;
    if(n + c < n)
      break;
    l = n + c;
    ++tmp;
  }
  if(tmp - src)
    *dest = l;
  return (size_t)(tmp - src);
}
