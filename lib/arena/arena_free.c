#include "../arena_internal.h"

void
arena_free(arena* a) {
  struct arena_chunk *c = a->chunk, *prev;

  for(; c; c = prev) {
    prev = c->prev;
    if(a->src && a->src->put)
      a->src->put(c, c->size);
  }

  a->chunk = NULL;
  a->beg = a->end = a->top = NULL;
}
