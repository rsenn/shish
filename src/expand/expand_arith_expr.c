#include "int64.h"
#include "tree.h"

/* expand a binary expression
 * ----------------------------------------------------------------------- */
int
expand_arith_expr(union node* expr, int64* r) {
  int ret;
  if(expr->id >= N_ARITH_UNARYMINUS) {
    ret = expand_arith_unary(&expr->narithunary, r);
  } else if(expr->id >= N_ARITH_OR) {
    ret = expand_arith_binary(&expr->narithbinary, r);
  }
  return ret;
}
