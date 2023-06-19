#include "../path_internal.h"

size_t
path_right(const char* s, size_t n) {
  const char* p = s + n - 1;
  while(p >= s && path_issep(*p))
    --p;
  while(p >= s && !path_issep(*p))
    --p;
  return p - s;
}
