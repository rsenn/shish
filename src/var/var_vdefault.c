#include "str.h"
#include "var.h"

/* return the variable value if it is set, otherwise return def
 * ----------------------------------------------------------------------- */
const char *var_vdefault(const char *v, const char *def, unsigned long *lenp) {
  struct var *var;

  if((var = var_search(v, NULL))) {
    if(lenp) *lenp = var->sa.len - var->offset;
    return &var->sa.s[var->offset];
  }
  
  if(lenp) *lenp = str_len(def);
  return def;
}

