#include "../../lib/byte.h"
#include "../expand.h"
#include "../tree.h"
#include "../parse.h"

/* expand one N_ARG nodes to a stralloc (appending)
 * ----------------------------------------------------------------------- */
void
expand_catsa(union node* node, stralloc* sa, int flags) {
  union node tmpnode;
  union node* n = &tmpnode;

  byte_copy(&tmpnode.narg.stra, sizeof(stralloc), sa);
  expand_arg(node, &n, flags | X_NOSPLIT);
  byte_copy(sa, sizeof(stralloc), &tmpnode.narg.stra);
  expand_unescape(sa, parse_isesc);
  stralloc_nul(sa);
}
