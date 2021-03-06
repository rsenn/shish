#include "../../lib/byte.h"
#include "../expand.h"
#include "../tree.h"

/* expand one N_ARG node to a stralloc (stralloc is overwritten!!!)
 * ----------------------------------------------------------------------- */
void
expand_copysa(union node* node, stralloc* sa, int flags) {
  union node tmpnode;
  union node* n = &tmpnode;

  stralloc_init(&tmpnode.narg.stra);
  expand_arg(node, &n, flags | X_NOSPLIT);
  byte_copy(sa, sizeof(stralloc), &tmpnode.narg.stra);
  stralloc_nul(sa);
}
