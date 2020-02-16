#include "../../lib/byte.h"
#include "../expand.h"
#include "../tree.h"

/* expand one N_ARG node to a stralloc (stralloc is overwritten!!!)
 * ----------------------------------------------------------------------- */
void
expand_tosa(union node* node, stralloc* out) {
  union node* arg = 0; /*tree_newnode(N_ARG);
  stralloc_init(&arg->narg.stra);
**/
  expand_arg(node, &arg, X_NOSPLIT);

  if(arg->id == N_ARG) {
    stralloc* sa;
    sa = &arg->narg.stra;
    stralloc_copy(out, sa);
  }
}
