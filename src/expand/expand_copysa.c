#include "../../lib/byte.h"
#include "../expand.h"
#include "../tree.h"

/* expand one N_ARG node to a stralloc (stralloc is overwritten!!!)
 * ----------------------------------------------------------------------- */
void
expand_copysa(union node* node, stralloc* sa, int flags) {
  union node tmpnode, *n = &tmpnode;

  tmpnode.narg.flag = 0;
  stralloc_init(&tmpnode.narg.stra);

  /* X_NOSPLIT routes every chunk through expand_cat()'s non-splitting
     branch, which unescapes each literal chunk as it's appended. */
  expand_arg(node, &n, flags | X_NOSPLIT);
  byte_copy(sa, sizeof(stralloc), &tmpnode.narg.stra);
  stralloc_nul(sa);
}
