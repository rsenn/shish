#include <stdlib.h>
#include "var.h"

/* set a variable value
 * ----------------------------------------------------------------------- */
const char *var_setvint(const char *v, int i, int flags) {
  struct var *var;

  var = var_create(v, flags);
  
  var->flags |= flags;

  if(var->sa.a == 0)
    var->sa.s = NULL;
  
  stralloc_copyb(&var->sa, v, var->len);
  stralloc_catc(&var->sa, '=');
  stralloc_catlong(&var->sa, i);
  stralloc_nul(&var->sa);
  var->offset = var->len + 1;
  return var->sa.s;
}

