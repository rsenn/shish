#include "vartab.h"
#include <assert.h>

/* discards current var context and gets the parent
 * ----------------------------------------------------------------------- */
void vartab_pop(struct vartab *vartab) {
  assert(varstack == vartab);
  
  vartab_cleanup(vartab);
  
  /* finally leave this level */
  varstack = vartab->parent;
  
  assert(varstack);
  assert(vartab->parent != vartab);
}
