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

  if(!expand_arith_expr(ex, expr, &ret)) {

    len = fmt_longlong(buf, ret);

    stralloc_zero(&n->narg.stra);

    n = expand_cat(buf, len, ex->ptr, ex->flags);
  }

  return n;
}
