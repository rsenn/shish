#include "../arena_internal.h"
#include <stdint.h>

/* new chunk with room for size bytes at align; 0 on failure */
static int
arena_more(arena* a, size_t size, size_t align) {
  struct arena_chunk* c;
  size_t want = ARENA_HDR + align - 1;
  size_t got;

  if(!a->src || !a->src->get || size > SIZE_MAX - want)
    return 0;

  want += size;
  got = a->csize > want ? a->csize : want;

  if(!(c = a->src->get(&got)) || got < want)
    return 0;

  c->prev = a->chunk;
  c->size = got;
  a->chunk = c;
  a->beg = (char*)c + ARENA_HDR;
  a->end = (char*)c + got;
  a->top = NULL;
  return 1;
}

void*
arena_take(arena* a, size_t size, size_t align) {
  size_t pad;
  char* p;

  if(!size)
    size = 1;
  if(!align)
    align = 1;

  for(;;) {
    pad = -(uintptr_t)a->beg & (align - 1);

    if((size_t)(a->end - a->beg) >= pad && (size_t)(a->end - a->beg) - pad >= size)
      break;
    if(!arena_more(a, size, align))
      return NULL;
  }

  p = a->beg + pad;
  a->beg = p + size;
  a->top = p;
  return p;
}
