/**
 * @defgroup   arena
 * @brief      ARENA module.
 *
 * A bump allocator: allocations are handed out from a chain of
 * growable blocks and can't be freed individually -- the whole arena
 * is reclaimed at once, either kept around for reuse (arena_reset())
 * or released back to the system (arena_free()). This matches a
 * lifetime shape that comes up repeatedly in this codebase: an AST
 * (src/tree.h) is parsed all at once, evaluated, then thrown away as
 * a whole, one malloc()/free() pair per node today
 * (src/tree/tree_newnode.c, src/tree/tree_free.c) where one region
 * create/destroy pair would do. See TODO.md's "Goal 4" for the full
 * writeup, including which call sites would own their own arena
 * (nested, strictly stack-disciplined -- shish is single-threaded and
 * synchronous, so arenas never need to overlap without nesting) and
 * which allocations (long-lived function/trap bodies, anything
 * variable-length like a stralloc buffer) deliberately stay off the
 * arena and continue through the plain lib/alloc.h heap.
 *
 * Interface only -- not implemented yet.
 * @{
 */
#ifndef ARENA_H
#define ARENA_H

#include <stdlib.h>

struct arena_block;

/* arena is a chain of arena_blocks; arena_alloc() bump-allocates from
 * the current (head) block, appending a new block once the current
 * one runs out of room. Nothing handed back by arena_alloc() can be
 * freed on its own -- see arena_reset()/arena_free() below for the
 * only two ways memory ever comes back. Zero-initialize (or use
 * arena_init()) before first use. */
typedef struct arena_s {
  struct arena_block* head;
  size_t blocksize; /* size used for each block appended on demand */
} arena;

/* arena_init sets up an empty arena. blocksize is the size of each
 * block appended on demand as arena_alloc() runs out of room in the
 * current one; 0 selects an implementation-chosen default. No memory
 * is actually allocated until the first arena_alloc() call. */
void arena_init(arena* a, size_t blocksize);

/* arena_alloc bump-allocates len bytes from a, appending a fresh block
 * if the current one doesn't have enough room left (an allocation
 * bigger than the arena's blocksize gets its own oversized block, so
 * there's no hard upper limit on a single arena_alloc() call). Returns
 * NULL only if the underlying system allocation fails; unlike
 * lib/alloc.h's alloc(), it does not exit(1) on its own. The returned
 * memory is valid until the next arena_reset()/arena_free() call on
 * the same arena -- there is no per-allocation free. */
void* arena_alloc(arena* a, size_t len);

/* arena_reset forgets every allocation made so far but keeps the
 * underlying blocks (and their capacity) around for reuse -- the next
 * arena_alloc() call reuses that capacity from the start instead of
 * asking the system for new memory. Meant for a workload that repeats
 * with a similar shape each time, e.g. one arena reset per parsed
 * statement in the interactive loop, rather than a fresh arena_init()
 * (and fresh block allocations) every time. */
void arena_reset(arena* a);

/* arena_free releases every block a owns back to the system. Leaves a
 * in the same state arena_init() would have, safe to reuse or to
 * simply go out of scope afterward. */
void arena_free(arena* a);

#endif
/** @} */
