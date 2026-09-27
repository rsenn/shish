#include "../utf8.h"

/* bytes taken up by the first k characters of s[0..n) */
size_t
text_charskip(int utf8, const char* s, size_t n, size_t k) {
  return utf8 ? u8skip(s, n, k) : k < n ? k : n;
}
