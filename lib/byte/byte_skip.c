#include "../byte.h"

/* returns the length of the run of c at the start of p[0..n) */
size_t
byte_skip(const void* p, size_t n, char c) {
  const char *s = (const char*)p, *t = s + n;

  while(s != t && *s == c)
    ++s;

  return s - (const char*)p;
}
