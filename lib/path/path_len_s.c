#include "../path_internal.h"

size_t
path_len_s(const char* s) {
  const char* p = s;
  while(*p && !path_issep(*p)) ++p;
  return p - s;
}
