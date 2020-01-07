#include "expand.h"
#include "fmt.h"
#include "tree.h"

/* expand an exprmetic expression
 * ----------------------------------------------------------------------- */
union node*
expand_arith(union node* arith, union node** nptr, int flags) {
  union node* expr = arith->nargarith.tree;
  union node* n = *nptr;
  int ret = -1;
  size_t len;
  char buf[FMT_LONG];

  if(expand_arith_expr(expr, &ret))
    return NULL;
  len = fmt_longlong(buf, ret);

  /* if there isn't already a node create one now! */
  /*  if(n == NULL) {
      *nptr = n = tree_newnode(N_ARG);
      nptr = &n;
      stralloc_init(&n->narg.stra);
    }*/

  n = expand_cat(buf, len, nptr, flags);
}
