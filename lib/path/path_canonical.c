#include "../path_internal.h"

int
path_canonical(const char* path, stralloc* out) {
  stralloc_zero(out);
  stralloc_copys(out, path);
  stralloc_nul(out);
  return path_canonical_sa(out);
}