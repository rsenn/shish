#include "tree.h"
#include "uint64.h"

/* expand a binary expression
 * ----------------------------------------------------------------------- */
int
expand_arith_expr(struct expand* ex, union node* expr, int64* r) {
  int ret = 0;

  if(expr->id == N_ARITH_PAREN) {
    ret = expand_arith_expr(ex, ((struct narithunary*)expr)->node, r);
  } else if(expr->id >= N_ARITH_UNARYMINUS) {
    ret = expand_arith_unary(ex, &expr->narithunary, r);
  } else if(expr->id >= N_ARITH_OR) {
    ret = expand_arith_binary(ex, &expr->narithbinary, r);
  } else if(expr->id == N_ARITH_NUM) {
    *r = ((struct narithnum*)expr)->num;
  } else if(expr->id == N_ARGPARAM) {
    union node* n = NULL;
    expand_param(ex, (struct nargparam*)expr, &n, 0);
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
