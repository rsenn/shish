#include "../arena_internal.h"

void
arena_trim(arena* a, void* p, size_t oldsize, size_t newsize) {
  if(p && newsize < oldsize && (char*)p == a->top && (char*)p + oldsize == a->beg)
    a->beg = (char*)p + newsize;
}
