#include "../sh.h"
#include "../vartab.h"
#include <stdlib.h>

/* set the flags if a variable is found on the current level
 * otherwise create a new one
 * ----------------------------------------------------------------------- */
int
var_chflg(char* v, int flags, int set) {
  struct var* var;

  if(!(var = var_create(v, 0)))
    return 0;

  /* if we found it on the current level
     then we just set the flags */
  if(var->table == sh->varstack) {
    if(set)
      var->flags |= flags;
    else
      var->flags &= (~flags);

  } else {
    /* otherwise create a new entry on the current level */
    if(set)
      flags = var->flags | flags;
    else
      flags = var->flags & (~flags);

    var_set(var->sa.s, flags);
  }

  return 1;
}
