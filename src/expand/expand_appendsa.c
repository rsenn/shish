#include "byte.h"
#include "expand.h"
#include "tree.h"

/* expand one N_ARG nodes to a stralloc (appending)
 * ----------------------------------------------------------------------- */
void
expand_appendsa(union node* node, stralloc* sa) {
  struct narg tmpnode = {N_ARG, 0, 0, 0};
  struct expand x = {(union node*)&tmpnode, &x.root, X_NOSPLIT};
  x.ptr = &x.root;

  //  byte_copy(&tmpnode.stra, sizeof(stralloc), sa);
  stralloc_init(&tmpnode.stra);
  stralloc_copy(&tmpnode.stra, sa);
  expand_arg(&x, &node->narg);
  byte_copy(sa, sizeof(stralloc), &tmpnode.stra);
  expand_unescape(sa);
  stralloc_nul(sa);
}
