#include "../utf8.h"

/* characters in s[0..n) */
size_t
text_charcount(int utf8, const char* s, size_t n) {
  return utf8 ? u8count(s, n) : n;
}
