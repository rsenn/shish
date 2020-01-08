#include "expand.h"
#include "tree.h"
#include <stdlib.h>

/* expand an assignment list
 * ----------------------------------------------------------------------- */
int
expand_vars(union node* vars, union node** nptr) {
  union node* var;
  union node* n;
  int ret = 0;
  struct expand x = EXPAND_INIT(0, 0, X_NOSPLIT);

  *x.ptr = NULL;

  for(var = vars; var; var = var->list.next) {
    if((n = expand_arg(&x, &var->narg))) {
      x.ptr = &n;
      ret++;
    }

    expand_unescape(&n->narg.stra);

    if(n)
      x.ptr = &n->list.next;
  }

  return ret;
}
