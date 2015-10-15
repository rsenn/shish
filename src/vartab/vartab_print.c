#include "vartab.h"
#include "var.h"

/* print all variables having the specified flags set 
 * ----------------------------------------------------------------------- */
void vartab_print(int flags) {
  struct var *var;

  for(var = var_list; var; var = var->gnext)
    if(var->child == NULL && (var->flags & flags) == flags)
      var_print(var, flags);
}

