#include "expand.h"
#include "scan.h"
#include "tree.h"
#include "uint64.h"

/* expand a binary expression
 * ----------------------------------------------------------------------- */
int
expand_arith_expr(struct expand* ex, union node* expr, int64* r) {
  int ret = 0;

  if(expr->id == A_PAREN) {
    ret = expand_arith_expr(ex, ((struct narithunary*)expr)->node, r);
  } else if(expr->id >= A_UNARYMINUS) {
    ret = expand_arith_unary(ex, &expr->narithunary, r);
  } else if(expr->id >= A_OR) {
    ret = expand_arith_binary(ex, &expr->narithbinary, r);
  } else if(expr->id == A_NUM) {
    *r = ((struct narithnum*)expr)->num;
  } else if(expr->id == N_ARGPARAM) {
    union node* n = expand_param(ex, (struct nargparam*)expr);
    size_t len = 0;
    if(n) {
      stralloc* value = &n->narg.stra;
      len = scan_longlong(value->s, r);
    }
    if(len == 0)
      return -1;
  } else {
    __asm__("int $3");
  }
  return ret;
}
