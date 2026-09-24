#include "../arena_internal.h"
#include "../byte.h"

void*
arena_grow(arena* a, void* p, size_t oldsize, size_t newsize) {
  if(!p || (char*)p != a->top || (char*)p + oldsize != a->beg)
    return NULL;

  if(newsize <= oldsize)
    return p;

  if((size_t)(a->end - (char*)p) < newsize)
    return NULL;

  a->beg = (char*)p + newsize;
  byte_zero((char*)p + oldsize, newsize - oldsize);
  return p;
}
