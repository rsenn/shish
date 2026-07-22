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

  /* X_NOSPLIT always routes every chunk through expand_cat()'s
     non-splitting branch, which now unescapes each literal chunk
     itself as it's appended (fixes/70) -- nothing left to do here but
     copy out the already-final buffer and nul-terminate it. */
  expand_arg(node, &n, flags | X_NOSPLIT);
  byte_copy(sa, sizeof(stralloc), &tmpnode.narg.stra);
  stralloc_nul(sa);
}
