#include "../../lib/byte.h"
#include "../vartab.h"
#include <assert.h>

/* push variable table to the stack
 * ----------------------------------------------------------------------- */
void
vartab_push(struct vartab* vartab, int function) {
  /* zero it */
  byte_zero(vartab, sizeof(struct vartab));

  assert(vartab != varstack);

  /* link it into the stack */
  vartab->parent = varstack;
  vartab->level = varstack->level + 1;
  vartab->function = function;
  varstack = vartab;
}
