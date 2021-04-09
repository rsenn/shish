#include "../expand.h"
#include "../../lib/fmt.h"
#include "../tree.h"

/* expand an arithmetic expression
 * ----------------------------------------------------------------------- */
union node*
expand_arith(struct nargarith* arith, union node** nptr) {
  union node *expr = arith->tree, *n = *nptr;
  int64 ret = -1;
  size_t len;
  char buf[FMT_LONG];

  if(!expand_arith_expr(expr, &ret)) {
    len = fmt_longlong(buf, ret);
   // stralloc_zero(&n->narg.stra);
    n = expand_cat(buf, len, &n, 0);
  }

  return n;
}
