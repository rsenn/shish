#include "expand.h"
#include "scan.h"
#include "tree.h"
#include "uint64.h"
#include <assert.h>

/* expand a binary expression
 * ----------------------------------------------------------------------- */
int
expand_arith_expr(struct expand* ex, union node* expr, int64* r) {
  int ret = 0;

  switch(expr->id) {
    case A_PAREN: ret = expand_arith_expr(ex, ((struct narithunary*)expr)->node, r); break;

    case A_UNARYMINUS:
    case A_UNARYPLUS:
    case A_NOT:
    case A_BNOT: ret = expand_arith_unary(ex, &expr->narithunary, r); break;
    case A_OR:
    case A_AND:
    case A_BOR:
    case A_BXOR:
    case A_BAND:
    case A_EQ:
    case A_NE:
    case A_LT:
    case A_GT:
    case A_GE:
    case A_LE:
    case A_LSHIFT:
    case A_RSHIFT:
    case A_ADD:
    case A_SUB:
    case A_MUL:
    case A_DIV:
    case A_MOD:
    case A_EXP: ret = expand_arith_binary(ex, &expr->narithbinary, r); break;
    case A_NUM: *r = ((struct narithnum*)expr)->num; break;
    case N_ARGPARAM: {
      union node* n = expand_param(ex, (struct nargparam*)expr);
              stralloc* value;
      size_t len = 0;
      assert(n);
       value = &n->narg.stra;
       assert(value);
       if(n && value->s)
        len = scan_longlong(value->s, r);
      if(len == 0)
        return -1;
    }
    default: __asm__("int $3"); break;
  }
  return ret;
}
