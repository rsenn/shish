#include "../expand.h"
#include "../tree.h"
#include "../var.h"
#include "../../lib/uint64.h"
#include <math.h>

/* expand a assignment expression
 * ----------------------------------------------------------------------- */
int
expand_arith_assign(struct narithbinary* assign, int64* r) {
  int64 left = -1, right = -1;
  if(expand_arith_expr(assign->left, &left))
    return 1;
  if(expand_arith_expr(assign->right, &right))
    return 1;

  switch(assign->id) {
    case A_ASSIGN: left = right; break;
    case A_ASSIGNADD: left += right; break;
    case A_ASSIGNSUB: left -= right; break;
    case A_ASSIGNMUL: left *= right; break;
    case A_ASSIGNDIV: left /= right; break;
    case A_ASSIGNMOD: left %= right; break;
    case A_ASSIGNLSHIFT: left <<= right; break;
    case A_ASSIGNRSHIFT: left >>= right; break;
    case A_ASSIGNBAND: left &= right; break;
    case A_ASSIGNBXOR: left ^= right; break;
    case A_ASSIGNBOR: left |= right; break;
    default: return 1;
  }

  var_setvint(assign->left->nargparam.name, left, V_DEFAULT);
  *r = left;

  return 0;
}
