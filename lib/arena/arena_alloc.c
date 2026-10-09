#include "../arena_internal.h"
#include "../byte.h"

void*
arena_alloc(arena* a, size_t size, size_t align) {
  void* p;

  if((p = arena_take(a, size, align)))
    byte_zero(p, size);
  
  return p;
}
