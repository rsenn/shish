#include "var.h"

struct var *var_copys(const char *s, int flags) {
  struct var *var;
  
  /* find/create the variable */
  if((var = var_create(s, flags)) == NULL)
    return NULL;
  
  /* initialize stralloc if it was a non-allocated string */
  if(var->sa.a == 0)
     stralloc_init(&var->sa);
  
  /* now copy the var */
  stralloc_copys(&var->sa, s);
  stralloc_nul(&var->sa);
  
  if(var->sa.len > var->len)
    var->offset = var->len + 1;
  
  return var;
}

  
  
