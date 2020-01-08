#include "byte.h"
#include "expand.h"
#include "tree.h"

/* expand one N_ARG node to a stralloc (stralloc is overwritten!!!)
 * ----------------------------------------------------------------------- */
void
expand_tosa(union node* node, stralloc* sa) {
  union node tmpnode = {0, 0, 0};

  struct expand x = EXPAND_INIT(0, &tmpnode, X_NOSPLIT);
  //  x.ptr = &x.root;

  stralloc_init(&tmpnode.narg.stra);
  expand_arg(&x, &node->narg);
  byte_copy(sa, sizeof(stralloc), &tmpnode.narg.stra);
  stralloc_nul(sa);
}
