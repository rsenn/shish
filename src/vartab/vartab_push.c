#include "byte.h"
#include <assert.h>
#include "vartab.h"

/* push variable table to the stack
 * ----------------------------------------------------------------------- */
void vartab_push(struct vartab *vartab) {
  /* zero it */
  byte_zero(vartab, sizeof(struct vartab));

  assert(vartab != varstack);
  
  /* link it into the stack */
  vartab->parent = varstack;
  vartab->level = varstack->level + 1;
  varstack = vartab;
}

