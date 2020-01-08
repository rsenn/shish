#include "byte.h"
#include "expand.h"
#include "tree.h"

/* expand one N_ARG nodes to a stralloc (appending)
 * ----------------------------------------------------------------------- */
void
expand_appendsa(union node* node, stralloc* sa) {
  union node tmpnode = {N_ARG, 0, 0, 0};
  struct expand x = EXPAND_INIT(&tmpnode, &x.root, X_NOSPLIT);
  x.ptr = &x.root;

  //  byte_copy(&tmpnode.narg.stra, sizeof(stralloc), sa);
  stralloc_init(&tmpnode.narg.stra);
  stralloc_copy(&tmpnode.narg.stra, sa);
  expand_arg(&x, &node->narg);
  byte_copy(sa, sizeof(stralloc), &tmpnode.narg.stra);
  expand_unescape(sa);
  stralloc_nul(sa);
}
