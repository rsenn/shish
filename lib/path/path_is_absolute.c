#include "../path.h"

int
path_is_absolute(const char* p) {
  return path_is_absolute_b(p, str_len(p));
}
