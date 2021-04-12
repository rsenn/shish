#include "../expand.h"
#include "../tree.h"
#include "../../lib/uint64.h"
#include "../var.h"
#include <math.h>

/* expand a unary expression
 * ----------------------------------------------------------------------- */
int
expand_arith_unary(struct narithunary* expr, int64* r) {
  int64 value;
  struct nargparam* param = 0;
  int pre_post = expr->id >= A_PREDECR && expr->id <= A_POSTINCR;

  if(expand_arith_expr(expr->node, &value))
    return 1;

  if(pre_post) {
    param = &expr->node->nargparam;
    var_search(param->name, NULL);
  }

  switch(expr->id) {
    case A_UNARYMINUS: *r = 0 - value; break;
    case A_UNARYPLUS: *r = value; break;
    case A_PREINCR: *r = ++value; break;
    case A_POSTINCR: *r = value++; break;
    case A_PREDECR: *r = --value; break;
    case A_POSTDECR: *r = value--; break;
    case A_NOT: *r = !value; break;
    case A_BNOT: *r = ~value; break;
    default: break;
  }

  if(pre_post) {
    if(param && param->name)
      var_setvint(param->name, value, 0);
  }

  return 0;
}
