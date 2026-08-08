#include "../expand.h"
#include "../tree.h"

/* expand a ternary ("?:") expression
 * ----------------------------------------------------------------------- */
int
expand_arith_ternary(struct narithternary* expr, int64* r) {
  int64 cond;

  if(expand_arith_expr(expr->cond, &cond))
    return 1;

  /* only the taken branch is evaluated -- the other one can have side
     effects (assignments) that must not run, same rule as "&&"/"||"
     in expand_arith_binary.c. */
  return expand_arith_expr(cond ? expr->ontrue : expr->onfalse, r);
}
