#include <stdlib.h>
#include "vartab.h"

/* get a variable
 * ----------------------------------------------------------------------- */
const char *var_get(const char *v, unsigned long *offset) {
  struct var *var;
  if((var = var_search(v, NULL))) {
    if(offset) 
      *offset = var->offset;
    return var->sa.s;
  }
  return 0;
}


