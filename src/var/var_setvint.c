#include "../var.h"
#include "../sh.h"
#include "../fdtable.h"
#include <stdlib.h>

/* set a variable value
 * ----------------------------------------------------------------------- */
const char*
var_setvint(const char* v, int i, int flags) {
  struct var* var;

  var = var_create(v, flags);

  if(var->flags & V_READONLY) {
    sh_msgn(var->sa.s, var->len);
    buffer_putsflush(fd_err->w, ": readonly variable\n");
    return 0;
  }

  var->flags |= flags;

  if(var->sa.a == 0)
    var->sa.s = NULL;

  stralloc_copyb(&var->sa, v, var->len);
  stralloc_catc(&var->sa, '=');
  stralloc_catlong(&var->sa, i);
  stralloc_nul(&var->sa);
  var->offset = var->len + 1;
  var->flags &= ~V_UNSET;
  return var->sa.s;
}
