#include "shell.h"
#include "var.h"

/* unset a variable
 * ----------------------------------------------------------------------- */
void var_unset(char *v) {
  struct var *var;
  
  /* find the variable */
  if((var = var_search(v, NULL)) == NULL)
    return;
  
  do {
/*    if(var->child && 
     (var->child->flags & V_FREE))
      shell_free(var->child);*/
    
    var_cleanup(var);
  } while((var = var->parent));
}

