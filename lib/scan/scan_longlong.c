#include "../scan.h"

static const unsigned long maxlong = ((unsigned long)-1) >> 1;

size_t
scan_longlong(const char* src, int64* dest) {
  size_t i, o;
  uint64 l;
  char c = src[0];
  unsigned int neg = c == '-';
  o = c == '-' || c == '+';
  if((i = scan_ulonglong(src + o, &l))) {
    if(i > 0 && l > maxlong + neg) {
      l /= 10;
      --i;
    }
    if(i + o)
      *dest = (int64)(c == '-' ? -l : l);
    return i + o;
  }
  return 0;
}
