#include "../path_internal.h"
#include "../utf8.h"

size_t
path_lenb(const char* s, size_t n) {
  return path_comlen(s, n);
}
