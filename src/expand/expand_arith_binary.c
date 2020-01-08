#include "expand.h"
#include "tree.h"
#include "uint64.h"
#include <math.h>

/* expand a binary expression
 * ----------------------------------------------------------------------- */
int
expand_arith_binary(struct expand* ex, struct narithbinary* expr, int64* r) {
  int64 left = -1, right = -1;
  if(expand_arith_expr(ex, expr->left, &left))
    return 1;
  if(expand_arith_expr(ex, expr->right, &right))
    return 1;

  switch(expr->id) {

    case N_ARITH_OR: *r = left || right; break;
    case N_ARITH_AND: *r = left && right; break;
    case N_ARITH_BOR: *r = left | right; break;
    case N_ARITH_BXOR: *r = left ^ right; break;
    case N_ARITH_BAND: *r = left & right; break;
    case N_ARITH_EQ: *r = left == right; break;
    case N_ARITH_NE: *r = left != right; break;
    case N_ARITH_LT: *r = left < right; break;
    case N_ARITH_GT: *r = left > right; break;
    case N_ARITH_GE: *r = left >= right; break;
    case N_ARITH_LE: *r = left <= right; break;
    case N_ARITH_LSHIFT: *r = left << right; break;
    case N_ARITH_RSHIFT: *r = left >> right; break;
    case N_ARITH_ADD: *r = left + right; break;
    case N_ARITH_SUB: *r = left - right; break;
    case N_ARITH_MUL: *r = left * right; break;
    case N_ARITH_DIV: *r = left / right; break;
    case N_ARITH_MOD: *r = left % right; break;
    case N_ARITH_EXP: *r = powl(left, right); break;
    default: __asm__("int $3"); break;
  }

  return 0;
}
