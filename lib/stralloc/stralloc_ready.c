#include "../alloc.h"
#include "../byte.h"
#include "../stralloc.h"
#include "../shell.h"

/* stralloc_ready makes sure that sa has enough space allocated to hold
 * len bytes: If sa is not allocated, stralloc_ready allocates at least
 * len bytes of space, and returns 1. If sa is already allocated, but
 * not enough to hold len bytes, stralloc_ready allocates at least len
 * bytes of space, copies the old string into the new space, frees the
 * old space, and returns 1. Note that this changes sa.s. */
int
stralloc_ready(stralloc* sa, size_t len) {
  size_t wanted = len + (len >> 3) + 30; /* heuristic from djb */
  char* tmp;

  if(sa->s && sa->a >= len)
    return 1;

  if(sa->a == 0 || sa->s == NULL) {
    if(!(tmp = alloc(wanted)))
      return 0;

    /* sa->a == 0 with sa->s already non-NULL means sa->s aliases a
       buffer stralloc doesn't own (e.g. var_set() pointing var->sa.s
       straight at an environment string) -- only sa->len bytes of it
       are actually ours to preserve. Copying `len` (the *new*,
       larger target capacity) instead read past wherever that
       aliased buffer's real allocation ends, into whatever memory
       happens to follow it (stralloc-ready-copies-past-aliased-
       buffer-end). */
    if(sa->s)
      byte_copy(tmp, sa->len, sa->s);
    else
      byte_zero(tmp, wanted);
  } else {
    if(!(tmp = alloc_re(sa->s, wanted)))
      return 0;
  }
  sa->a = wanted;
  sa->s = tmp;
  return 1;
}
