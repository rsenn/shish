#include "../alloc.h"
#include "../shell.h"
#include "../stralloc.h"

#ifdef DEBUG_ALLOC

/* stralloc_ready makes sure that sa has enough space allocated to hold
 * len bytes: If sa is not allocated, stralloc_ready allocates at least
 * len bytes of space, and returns 1. If sa is already allocated, but
 * not enough to hold len bytes, stralloc_ready allocates at least len
 * bytes of space, copies the old string into the new space, frees the
 * old space, and returns 1. Note that this changes sa.s. */
int
stralloc_readydebug(const char* file, unsigned int line, stralloc* sa, unsigned long int len) {
  register int wanted = len + (len >> 3) + 30; /* heuristic from djb */
  if(!sa->s || sa->a < len) {
    register char* tmp;

    if(!(tmp = alloc_redebug(file, line, sa->s, wanted)))
      return 0;
    sa->a = wanted;
    sa->s = tmp;
  }
  return 1;
}
#endif /* defined(DEBUG_ALLOC) */
