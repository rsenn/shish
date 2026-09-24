#include "../arena_internal.h"
#include "../byte.h"

void
arena_init(arena* a, const struct arena_src* src, size_t chunk) {
  byte_zero(a, sizeof(*a));
  a->src = src;
  a->csize = chunk ? chunk : ARENA_CHUNK;
}
