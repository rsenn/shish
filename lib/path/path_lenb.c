#include "internal.h"
#include "../utf8.h"

size_t
path_lenb(const char* s, size_t n) {
  return COMLEN(s, n);
}
