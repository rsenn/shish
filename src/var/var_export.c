#include <stdlib.h>
#include "byte.h"
#include "var.h"

/* export variables for execve() 
 * ----------------------------------------------------------------------- */
char **var_export(char **dest) {
  struct var *var;
  unsigned long n = 0;

  for(var = var_list; var; var = var->gnext)
    if((var->flags & V_EXPORT))
      dest[n++] = var->sa.s;

  dest[n] = NULL;
  
  return dest;
}


