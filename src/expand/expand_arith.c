#include "expand.h"
#include "fmt.h"
#include "tree.h"

/* expand an exprmetic expression
 * ----------------------------------------------------------------------- */
union node*
expand_arith(struct expand* ex, union node* arith) {
  union node* expr = arith->nargarith.tree;
  union node* n = ex->ptr;
  int ret = -1;
  size_t len;
  char buf[FMT_LONG];

  if(expand_arith_expr(ex, expr, &ret))
    return NULL;
  len = fmt_longlong(buf, ret);

  /* if there isn't already a node create one now! */
  /*  if(n == NULL) {
      *ex->ptr = n = tree_newnode(N_ARG);
      ex->ptr = &n;
      stralloc_init(&n->narg.stra);
    }*/

  n = expand_cat(&ex, buf, len);
}
