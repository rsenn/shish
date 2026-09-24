#ifndef ARENA_INTERNAL_H
#define ARENA_INTERNAL_H

#include "arena.h"

/* header at the start of every chunk (fixed buffers get one too) */
struct arena_chunk {
  struct arena_chunk* prev;
  size_t size; /* whole chunk incl. this header */
};

#define ARENA_HDR ((sizeof(struct arena_chunk) + 15) & ~(size_t)15)
#define ARENA_CHUNK 8192

/* unzeroed allocation; align must be a power of two */
void* arena_take(arena* a, size_t size, size_t align);

#endif
