#include "fdstack.h"

/* pushes a copy of the current io context to the fdtable
 * ----------------------------------------------------------------------- */
void fdstack_push(struct fdstack *st)
{
  assert(st < fdstack || fdstack == &fdstack_root);
  
  /* set up the new i/o table */
  st->list = NULL;
  st->parent = fdstack;
  st->level = fdstack->level + 1;
  
  /* now set the new stack top */
  fdstack = st;
}
