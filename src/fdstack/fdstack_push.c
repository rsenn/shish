#include "../fdstack.h"
#include <assert.h>

/* pushes a copy of the current io context to the fdtable
 * ----------------------------------------------------------------------- */
void
fdstack_push(struct fdstack* st) {
  /* catch pushing the same frame that's already the current top
     (mirrors vartab_push()'s "vartab != varstack" check) -- this used
     to instead compare raw addresses ("st < fdstack"), assuming
     normal downward stack growth so a callee's (deeper) frame always
     sits below its caller's. That's not a portable invariant: under
     AddressSanitizer's stack-use-after-return detection in particular,
     "escaping" locals like this one get allocated on a separate fake
     stack, in no particular order relative to each other, so the
     comparison fired as a false positive for any redirection at all
     inside a command substitution (fdstack-push-assertion-cmdsubst-
     redir) despite nothing actually being wrong. */
  assert(st != fdstack);

  /* set up the new i/o table */
  st->list = NULL;
  st->parent = fdstack;
  st->level = fdstack->level + 1;

  /* now set the new stack top */
  fdstack = st;
}
