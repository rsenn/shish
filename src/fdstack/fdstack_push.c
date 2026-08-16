#include "../fdstack.h"
#include <assert.h>

/* pushes a copy of the current io context to the fdtable
 * ----------------------------------------------------------------------- */
void
fdstack_push(struct fdstack* st) {
  /* catch pushing the same frame that's already the current top
     (mirrors vartab_push()'s "vartab != varstack" check). Compares by
     identity, not by stack address: relative address order between
     frames isn't portable (e.g. ASan's stack-use-after-return puts
     "escaping" locals on a separate fake stack). */
  assert(st != fdstack);

  /* set up the new i/o table */
  st->list = NULL;
  st->parent = fdstack;
  st->level = fdstack->level + 1;

  /* now set the new stack top */
  fdstack = st;
}
