#include "expand.h"
#include "fmt.h"
#include "tree.h"

/* expand an exprmetic expression
 * ----------------------------------------------------------------------- */
union node*
expand_arith(struct expand* ex, struct nargarith* arith) {
  union node* expr = arith->tree;
  union node* n = *ex->ptr;
  int64 ret = -1;
  size_t len;
  char buf[FMT_LONG];

  /* if there isn't already a node create one now! */
  if(n == NULL) {
     n =  *ex->ptr = tree_newnode(N_ARG);
      stralloc_init(&n->narg.stra);
  }

  if(expand_arith_expr(ex, expr, &ret))
    return NULL;

  len = fmt_longlong(buf, ret);

  n = expand_cat(buf, len, ex->ptr, ex->flags);

  return n;
}
