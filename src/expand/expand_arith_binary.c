#include "../expand.h"
#include "../tree.h"
#include "../../lib/uint64.h"
#include <math.h>
#include <assert.h>

/* expand a binary expression
 * ----------------------------------------------------------------------- */
int
expand_arith_binary(struct narithbinary* expr, int64* r) {
  int64 left = -1, right = -1;
  if(expand_arith_expr(expr->left, &left))
    return 1;
  if(expand_arith_expr(expr->right, &right))
    return 1;

  switch(expr->id) {
    case A_OR: *r = left || right; break;
    case A_AND: *r = left && right; break;
    case A_BITOR: *r = left | right; break;
    case A_BITXOR: *r = left ^ right; break;
    case A_BITAND: *r = left & right; break;
    case A_EQ: *r = left == right; break;
    case A_NE: *r = left != right; break;
    case A_LT: *r = left < right; break;
    case A_GT: *r = left > right; break;
    case A_GE: *r = left >= right; break;
    case A_LE: *r = left <= right; break;
    case A_SHL: *r = left << right; break;
    case A_SHR: *r = left >> right; break;
    case A_ADD: *r = left + right; break;
    case A_SUB: *r = left - right; break;
    case A_MUL: *r = left * right; break;
    case A_DIV: *r = left / right; break;
    case A_MOD: *r = left % right; break;
    case A_EXP: *r = powl(left, right); break;
    default: 
assert(0);
     break;
  }

  return 0;
}
