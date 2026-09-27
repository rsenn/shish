#include "../utf8.h"

/* bytes of the next character: utf8 selects UTF-8 (u8charlen), else one byte */
size_t
text_charlen(int utf8, const char* s, size_t n) {
  return utf8 ? u8charlen(s, n) : n ? 1 : 0;
}
