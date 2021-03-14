#include "../../lib/byte.h"
#include "../var.h"
#include <assert.h>

/* set a variable value
 * ----------------------------------------------------------------------- */
const char*
var_setv(const char* name, const char* value, size_t vlen, int flags) {
  struct var* var;

  /* find/create new variable on top vartab */
  var = var_create(name, flags);

  /* variable currently has no controlable stralloc.. . */
  if(var->sa.a == 0) {
    stralloc_init(&var->sa);
    stralloc_cats(&var->sa, name);
    var->offset = var->sa.len;
    var->flags |= V_FREESTR;
  }

  /* variable has a stralloc of which we have control */
  assert(var->flags & V_FREESTR);
  assert(var->sa.a);

  var->sa.len = var->offset;
  stralloc_catc(&var->sa, '=');
  stralloc_catb(&var->sa, value, vlen);
  // stralloc_trunc(&var->sa, var->sa.len);

  /* set flags */
  var->flags |= flags;
  return var->sa.s;
}
