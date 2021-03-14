#include "../expand.h"
#include "../tree.h"
#include "../parse.h"

#include <stdlib.h>

/* expand an assignment list
 * ----------------------------------------------------------------------- */
int
expand_vars(union node* vars, union node** nptr) {
  union node* var;
  union node* n;
  int ret = 0;

  *nptr = NULL;

  for(var = vars; var; var = var->list.next) {
    if((n = expand_arg(var, nptr, X_NOSPLIT))) {
      nptr = &n;
      ret++;
    }

    expand_unescape(&n->narg.stra, parse_isesc);

    if(n)
      nptr = &n->list.next;
  }

  return ret;
}
