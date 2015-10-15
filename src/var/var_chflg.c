#include <stdlib.h>
#include "sh.h"
#include "vartab.h"

/* set the flags if a variable is found on the current level 
 * otherwise create a new one
 * ----------------------------------------------------------------------- */
int var_chflg(char *v, int flags, int set) {
  struct var *var;
  
  var = var_create(v, 0);
  
  /* if we found it on the current level 
     then we just set the flags */
  if(var->table == sh->varstack) {
    if(set)
      var->flags |= flags;
    else
      var->flags &= (~flags);

    return 1;
  }
  /* otherwise create a new entry on the current level */
  else {
    if(set)
      flags = var->flags | flags;
    else
      flags = var->flags & (~flags);

    var_set(var->sa.s, flags);
    return 1;
  }
  
  return 0;
}

