#include "../str.h"

/* str_start returns 1 if the b is a prefix of a, 0 otherwise */
int
str_start(const char* a, const char* b) {
  return str_startb(a, b, str_len(b));
}
