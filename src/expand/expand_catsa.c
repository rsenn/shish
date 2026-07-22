#include "../../lib/byte.h"
#include "../expand.h"
#include "../tree.h"
#include "../parse.h"

/* expand one N_ARG nodes to a stralloc (appending)
 * ----------------------------------------------------------------------- */
void
expand_catsa(union node* node, stralloc* sa, int flags) {
  union node tmpnode, *n = &tmpnode;

  tmpnode.narg.flag = 0;
  byte_copy(&tmpnode.narg.stra, sizeof(stralloc), sa);
  expand_arg(node, &n, flags | X_NOSPLIT);
  byte_copy(sa, sizeof(stralloc), &tmpnode.narg.stra);

  if(tmpnode.narg.flag & X_LITERAL)
    expand_unescape(sa, parse_isesc);

  stralloc_nul(sa);
}
