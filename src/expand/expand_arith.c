#include "../expand.h"
#include "../../lib/fmt.h"
#include "../tree.h"

/* expand an arithmetic expression
 * ----------------------------------------------------------------------- */
union node*
expand_arith(struct nargarith* arith, union node** nptr, int flags) {
  union node *expr = arith->tree, *n = *nptr;
  int64 ret = -1;
  size_t len;
  char buf[FMT_LONG];

  if(!expand_arith_expr(expr, &ret)) {
    len = fmt_longlong(buf, ret);
    n = expand_cat(buf, len, &n, flags);
  }

  return n;
}
