#include "../arena_internal.h"
#include "../byte.h"
#include <stdint.h>

void
arena_init_fixed(arena* a, void* buf, size_t len) {
  size_t pad = -(uintptr_t)buf & 15;
  struct arena_chunk* c = (struct arena_chunk*)((char*)buf + pad);

  byte_zero(a, sizeof(*a));

  if(len < pad || len - pad < ARENA_HDR)
    return;

  len -= pad;
  c->prev = NULL;
  c->size = len;
  a->chunk = c;
  a->beg = (char*)c + ARENA_HDR;
  a->end = (char*)c + len;
}
