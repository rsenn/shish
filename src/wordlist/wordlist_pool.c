#include "../wordlist.h"
#include "../../lib/alloc.h"

/* one buffer per nesting depth ($(...) inside an expansion, a function call inside a command):
 * steady state does no malloc, a buffer keeps the capacity of its last use. Taken and
 * returned like a stack; the array grows and is never shrunk.
 * ----------------------------------------------------------------------- */
static stralloc** pool;
static size_t pool_used, pool_cap;

stralloc*
wordlist_pool_get(void) {
  if(pool_used == pool_cap) {
    size_t cap = pool_cap ? pool_cap * 2 : 8;
    stralloc** p = alloc_re(pool, cap * sizeof(stralloc*));

    if(p == NULL)
      return NULL;

    pool = p;
    pool_cap = cap;

    while(cap-- > pool_used)
      pool[cap] = NULL;
  }

  if(pool[pool_used] == NULL && (pool[pool_used] = alloc_zero(sizeof(stralloc))) == NULL)
    return NULL;

  return pool[pool_used++];
}

int
wordlist_pool_put(stralloc* sa) {
  if(pool_used == 0 || sa != pool[pool_used - 1])
    return 0;

  pool_used--;
  return 1;
}

size_t
wordlist_pool_mark(void) {
  return pool_used;
}

void
wordlist_pool_release(size_t mark) {
  if(mark < pool_used)
    pool_used = mark;
}
