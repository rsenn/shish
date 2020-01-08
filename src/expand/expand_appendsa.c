#include "byte.h"
#include "expand.h"
#include "tree.h"

/* expand one N_ARG nodes to a stralloc (appending)
 * ----------------------------------------------------------------------- */
void
expand_appendsa(union node* node, stralloc* sa) {
union node tmpnode = {0};
  struct expand x = EXPAND_INIT(0, &tmpnode, X_NOSPLIT);

  byte_copy(&tmpnode.narg.stra, sizeof(stralloc), sa);
  expand_arg(&x, &node->narg);
  byte_copy(sa, sizeof(stralloc), &tmpnode.narg.stra);
  expand_unescape(sa);
  stralloc_nul(sa);
}
