#include "../../lib/byte.h"
#include "expand.h"
#include "tree.h"

/* expand one N_ARG node to a stralloc (stralloc is overwritten!!!)
 * ----------------------------------------------------------------------- */
void
expand_tosa(union node* node, stralloc* sa) {
  struct narg tmpnode = {N_ARG, 0, 0, 0};
  union node* n = NULL;

  //  x.ptr = &x.root;

  stralloc_init(&tmpnode.stra);
  expand_arg(&node->narg, &n, X_NOSPLIT);
  byte_copy(sa, sizeof(stralloc), &tmpnode.stra);
  stralloc_nul(sa);
}
