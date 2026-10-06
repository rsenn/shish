#include "../wordlist.h"

/* one buffer per nesting depth ($(...) inside an expansion): steady state does no malloc,
 * the buffer keeps the capacity of the last command. Taken and returned like a stack.
 * ----------------------------------------------------------------------- */
static stralloc pool[WORDLIST_POOL];
static unsigned pool_used;

stralloc*
wordlist_pool_get(void) {
  return pool_used < WORDLIST_POOL ? &pool[pool_used++] : NULL;
}

int
wordlist_pool_put(stralloc* sa) {
  if(pool_used == 0 || sa != &pool[pool_used - 1])
    return 0;

  pool_used--;
  return 1;
}
