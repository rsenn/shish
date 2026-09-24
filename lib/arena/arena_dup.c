#include "../arena_internal.h"
#include "../byte.h"

void*
arena_dup(arena* a, const void* p, size_t len) {
  void* q = arena_take(a, len, 1);

  if(q)
    byte_copy(q, len, p);
  return q;
}
