#include "../utf8.h"

/* bytes taken up by the first k characters of s[0..n) (all n if there are fewer) */
size_t
u8skip(const char* s, size_t n, size_t k) {
  size_t off = 0;

  while(k && off < n) {
    off += u8charlen(s + off, n - off);
    k--;
  }

  return off;
}
