#include "../utf8.h"

/* bytes of the character at s: a valid sequence's length, else 1 (an invalid or
 * cut-off byte counts as a character of its own); 0 only when n is 0 */
size_t
u8charlen(const char* s, size_t n) {
  unsigned cp;
  int r = u8decode(s, n, &cp);

  return r > 0 ? (size_t)r : n ? 1 : 0;
}
