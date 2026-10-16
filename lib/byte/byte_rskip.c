#include "../byte.h"

/* returns the length of the run of c at the end of p[0..n) */
size_t
byte_rskip(const void* p, size_t n, char c) {
  const char* s = (const char*)p;
  size_t i = n;

  while(i && s[i - 1] == c)
    --i;

  return n - i;
}
