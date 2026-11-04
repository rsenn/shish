#include "walk_internal.h"
#include "../byte.h"

void*
walk_grow(arena* a, void* p, size_t oldsize, size_t newsize) {
  arena n;
  void* q;

  if((q = arena_grow(a, p, oldsize, newsize)))
    return q;

  arena_init(&n, &arena_heap, newsize * 2);

  if(!(q = arena_alloc(&n, newsize, 16))) {
    arena_free(&n);
    return NULL;
  }

  byte_copy(q, oldsize, p);
  arena_free(a);
  *a = n;
  return q;
}
