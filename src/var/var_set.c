#include "str.h"
#include "byte.h"
#include "var.h"

/* set a variable
 * ----------------------------------------------------------------------- */
struct var *var_set(char *v, int flags) {
  struct var *var;
  
  /* find/create the variable */
  if((var = var_create(v, flags)) == NULL)
    return var;
  
  /* free if it was a previously allocated string */
  if(var->sa.a)
    stralloc_free(&var->sa);
    
  stralloc_init(&var->sa);
  
  var->sa.s = v;
  var->sa.len = str_len(v);
  var->offset = var->len;
  
  if(var->len < var->sa.len)
    var->offset++;
  
  return var;
}

