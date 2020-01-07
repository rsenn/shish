#include "tree.h"
#include "uint64.h"

/* expand a binary expression
 * ----------------------------------------------------------------------- */
int
expand_arith_expr(union node* expr, int64* r) {
  int ret = 0;
  
  if(expr->id == N_ARITH_PAREN) {
    ret = expand_arith_expr(((struct narithunary*)expr)->node, r);
  } else if(expr->id >= N_ARITH_UNARYMINUS) {
    ret = expand_arith_unary(&expr->narithunary, r);
  } else if(expr->id >= N_ARITH_OR) {
    ret = expand_arith_binary(&expr->narithbinary, r);
  } else if(expr->id >= N_ARITH_NUM) {
    *r = ((struct narithnum*)expr)->num;
  } else {
    __asm__("int $3");
  }
  return ret;
}
