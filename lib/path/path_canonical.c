#include "../path_internal.h"

int
path_canonical(const char* path, stralloc* out) {
  if(!stralloc_copys(out, path))
    return 0;

  return path_canonical_sa(out);
}
