#include "../utf8.h"

/* characters in s[0..n) */
size_t
u8count(const char* s, size_t n) {
  size_t chars = 0;

  while(n) {
    size_t l = u8charlen(s, n);

    s += l;
    n -= l;
    chars++;
  }

  return chars;
}
