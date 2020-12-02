#include "../byte.h"

size_t
byte_equal(const void* s, size_t n, const void* t) {
  return byte_diff(s, n, t) == 0;
}
