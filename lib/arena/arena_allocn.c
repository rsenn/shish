#include "../arena_internal.h"
#include <stdint.h>

void*
arena_allocn(arena* a, size_t size, size_t n, size_t align) {
  if(n && size > SIZE_MAX / n)
    return NULL;
  return arena_alloc(a, size * n, align);
}
