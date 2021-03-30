#include "../../lib/byte.h"
#include "../expand.h"
#include "../tree.h"

/* expand one N_ARG node to a stralloc (stralloc is overwritten!!!)
 * ----------------------------------------------------------------------- */
void
expand_tosa(union node* node, stralloc* out) {
  union node* arg = 0;

  expand_arg(node, &arg, X_NOSPLIT);

  if(arg && arg->id == N_ARG) {
    stralloc* sa = &arg->narg.stra;
    stralloc_copy(out, sa);
  }
}
