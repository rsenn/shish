/**
 * @defgroup   arena
 * @brief      ARENA module.
 *
 * Bump allocator: allocations are never freed one by one; a whole
 * group is released at once with arena_rewind()/arena_reset()/arena_free().
 * Independent of the shell; the memory source is pluggable (arena_src).
 * @{
 */
#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

/* where chunks come from; the only part of the arena that touches the OS.
 *
 *   get   size in = minimum, size out = actual (>= minimum); NULL on failure
 *   put   gives a chunk back; NULL = chunks are never returned
 * ----------------------------------------------------------------------- */
struct arena_src {
  void* (*get)(size_t* size);
  void (*put)(void* p, size_t size);
};

extern const struct arena_src arena_heap; /* malloc()/free() */
extern const struct arena_src arena_mmap; /* anonymous mmap()/munmap() */
extern const struct arena_src arena_brk;  /* sbrk(); never returned */

struct arena_chunk;

typedef struct arena_s {
  char* beg;                        /* next free byte in the current chunk */
  char* end;                        /* one past the current chunk */
  char* top;                        /* start of the newest allocation, or NULL */
  struct arena_chunk* chunk;        /* current chunk; earlier ones chain back */
  const struct arena_src* src;      /* NULL = fixed buffer, never grows */
  size_t csize;                     /* preferred size of a new chunk */
} arena;                            /* all zero = empty fixed arena: every alloc fails */

/* setup and teardown
 * ----------------------------------------------------------------------- */

/* empty arena that pulls chunks from src; no memory until the first alloc.
 *
 *   arena*                   a      arena to set up
 *   const struct arena_src*  src    chunk source, e.g. &arena_heap
 *   size_t                   chunk  preferred chunk size, 0 = 8192
 * ----------------------------------------------------------------------- */
void arena_init(arena* a, const struct arena_src* src, size_t chunk);

/* arena inside a caller-owned buffer (stack array, alloca(), .bss); it never
 * grows, alloc returns NULL once len is used up. */
void arena_init_fixed(arena* a, void* buf, size_t len);

void arena_reset(arena* a);        /* forget all allocations, keep the first chunk */
void arena_free(arena* a);         /* give every chunk back via src->put, arena empty */
size_t arena_used(const arena* a); /* bytes handed out, plus slack of retired chunks */

/* fixed-size objects: zeroed, NULL when full or src->get fails (never exit)
 * ----------------------------------------------------------------------- */
void* arena_alloc(arena* a, size_t size, size_t align);
void* arena_allocn(arena* a, size_t size, size_t n, size_t align); /* NULL if n*size overflows */

#define arena_new(a, T) ((T*)arena_alloc((a), sizeof(T), __alignof__(T)))
#define arena_newn(a, T, n) ((T*)arena_allocn((a), sizeof(T), (n), __alignof__(T)))

/* frozen variable-length blobs: alignment 1, only the copy is written
 * ----------------------------------------------------------------------- */
void* arena_dup(arena* a, const void* p, size_t len);
char* arena_strndup(arena* a, const char* s, size_t len); /* appends the NUL */

/* growing the newest allocation
 * ----------------------------------------------------------------------- */

/* extends p in place and returns it; the new bytes are zeroed.
 * Returns NULL, changing nothing, unless p is the newest allocation
 * (oldsize as allocated; a size-0 alloc occupies 1 byte) and the rest of
 * its chunk has room for newsize. Never moves or copies: a copy would
 * leave a hole in the arena, so the caller decides how to recover. */
void* arena_grow(arena* a, void* p, size_t oldsize, size_t newsize);

/* freeze: returns the slack of p to the arena if p is still the newest */
void arena_trim(arena* a, void* p, size_t oldsize, size_t newsize);

/* nested lifetimes, strictly stack-disciplined
 * ----------------------------------------------------------------------- */
typedef struct {
  struct arena_chunk* chunk;
  char* beg;
} arena_pos;

arena_pos arena_tell(const arena* a);
void arena_rewind(arena* a, arena_pos pos); /* frees everything allocated since pos */

#endif
/** @} */
