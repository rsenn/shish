#include "../../lib/byte.h"
#include "../expand.h"
#include "../tree.h"

/* expand one N_ARG nodes to a stralloc (appending)
 * ----------------------------------------------------------------------- */
void
expand_catsa(union node* node, stralloc* sa, int flags) {
  union node tmpnode, *n = &tmpnode;

  /* X_NOSPLIT routes every chunk through expand_cat()'s non-splitting
     branch, which unescapes each literal chunk as it's appended --
     nothing left to do here but copy out the finished buffer. */
  tmpnode.narg.flag = 0;
  byte_copy(&tmpnode.narg.stra, sizeof(stralloc), sa);
  expand_arg(node, &n, flags | X_NOSPLIT);
  byte_copy(sa, sizeof(stralloc), &tmpnode.narg.stra);
  stralloc_nul(sa);
}
