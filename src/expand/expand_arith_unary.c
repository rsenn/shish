#include "tree.h"
#include "uint64.h"
#include <math.h>

/* expand a unary expression
 * ----------------------------------------------------------------------- */
int
expand_arith_unary(struct expand* ex, struct narithunary* expr, int64* r) {
  int64 value;

  if(expand_arith_expr(ex, expr->node, &value))
    return 1;

  switch(expr->id) {
    case N_ARITH_UNARYMINUS: *r = -value; break;
    case N_ARITH_UNARYPLUS: *r = +value; break;
    case N_ARITH_NOT: *r = !value; break;
    case N_ARITH_BNOT: *r = ~value; break;

    default: __asm__("int $3"); break;
  }

  return 0;
}
