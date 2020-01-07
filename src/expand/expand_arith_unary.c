#include "tree.h"
#include "uint64.h"
#include <math.h>

/* expand a unary expression
 * ----------------------------------------------------------------------- */
int
expand_arith_unary(struct narithunary* expr, int64* r) {
  int64 value;
  if(expand_arith_expr(expr->node, &value))
    return 1;

  switch(expr->id) {
    default: __asm__("int $3"); break;
  }

  return 0;
}
