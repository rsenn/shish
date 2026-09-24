#include "../arena_internal.h"

size_t
arena_used(const arena* a) {
  const struct arena_chunk* c = a->chunk;
  size_t n = 0;

  if(!c)
    return 0;

  n = a->beg - ((const char*)c + ARENA_HDR);

  for(c = c->prev; c; c = c->prev)
    n += c->size - ARENA_HDR;

  return n;
}
