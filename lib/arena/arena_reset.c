#include "../arena_internal.h"

void
arena_reset(arena* a) {
  struct arena_chunk* c;

  if(!(c = a->chunk))
    return;

  while(c->prev) {
    struct arena_chunk* prev = c->prev;

    if(a->src && a->src->put)
      a->src->put(c, c->size);
    c = prev;
  }

  a->chunk = c;
  a->beg = (char*)c + ARENA_HDR;
  a->end = (char*)c + c->size;
  a->top = NULL;
}
