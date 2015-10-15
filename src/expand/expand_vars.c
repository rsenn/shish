#include <stdlib.h>
#include "tree.h"
#include "expand.h"

/* expand an assignment list
 * ----------------------------------------------------------------------- */
int expand_vars(union node *vars, union node **nptr) {
  union node *var;
  union node *n;
  int ret = 0;

  *nptr = NULL;

  for(var = vars; var; var = var->list.next) {
    if((n = expand_arg(&var->narg, nptr, X_NOSPLIT))) {
      nptr = &n;
      ret++;
    }
    
    expand_unescape(&n->narg.stra);
    
    if(n)
      nptr = &n->list.next;
  }
  
  return ret;
}


