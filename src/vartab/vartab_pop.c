#include "../vartab.h"
#include <assert.h>
#include "../trace.h"

/* discards current var context and gets the parent
 * ----------------------------------------------------------------------- */
void
vartab_pop(struct vartab* vartab) {
  assert(varstack == vartab);

  TRACE(TRACE_VAR, "vartab.pop", trace_int("level", vartab->level), trace_int("function", vartab->function));

  vartab_cleanup(vartab);

  /* finally leave this level */
  varstack = vartab->parent;

  assert(varstack);
  assert(vartab->parent != vartab);
}
