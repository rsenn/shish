#include "../stralloc.h"
#undef stralloc_catlong

size_t
stralloc_catlong(stralloc* sa, long l) {
  return stralloc_catlong0(sa, l, 0);
}
