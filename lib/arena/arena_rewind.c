#include "../arena_internal.h"

void
arena_rewind(arena* a, arena_pos pos) {
  struct arena_chunk* c;

  /* taken while empty: keep the first chunk for reuse */
  if(!pos.chunk) {
    arena_reset(a);
    return;
  }

  while((c = a->chunk) != pos.chunk) {
    a->chunk = c->prev;
    if(a->src && a->src->put)
      a->src->put(c, c->size);
  }

  a->beg = pos.beg;
  a->end = (char*)c + c->size;
  a->top = NULL;
}
