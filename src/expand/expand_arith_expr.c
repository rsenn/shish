#include "../expand.h"
#include "../../lib/scan.h"
#include "../tree.h"
#include "../../lib/uint64.h"
#include <assert.h>

/* expand a binary expression
 * ----------------------------------------------------------------------- */
int
expand_arith_expr(union node* expr, int64* r) {
  int ret = 0;

  switch(expr->id) {
    case A_PAREN: {
      ret = expand_arith_expr(((struct narithunary*)expr)->node, r);
      break;
    }

    case A_UNARYMINUS:
    case A_UNARYPLUS:
    case A_NOT:
    case A_BNOT:
    case A_PREINCREMENT:
    case A_PREDECREMENT:
    case A_POSTINCREMENT:
    case A_POSTDECREMENT: {
      ret = expand_arith_unary(&expr->narithunary, r);
      break;
    }
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
    case A_EXP: {
      ret = expand_arith_binary(&expr->narithbinary, r);
      break;
    }
    case A_NUM: {
      *r = ((struct narithnum*)expr)->num;
      ret = 0;
      break;
    }
    case N_ARGPARAM: {
      union node* node = tree_newnode(N_ARG);
      union node* n = expand_param(&expr->nargparam, &node, 0);
      stralloc* value;
      size_t len = 0;
      assert(n);
      value = &n->narg.stra;
      assert(value);
      ret = len == 0;
      if(n && value->s) {
        len = scan_longlong(value->s, r);
      }
      ret = len == 0;
      if(ret)
        *r = 0;
      break;
    }
    case A_ASSIGN:
    case A_ASSIGNADD:
    case A_ASSIGNSUB:
    case A_ASSIGNMUL:
    case A_ASSIGNDIV:
    case A_ASSIGNMOD:
    case A_ASSIGNLSHIFT:
    case A_ASSIGNRSHIFT:
    case A_ASSIGNBAND:
    case A_ASSIGNBXOR:
    case A_ASSIGNBOR: {
      ret = expand_arith_assign(&expr->narithbinary, r);
      break;
    }
    default: {
      ret = -1;
      break;
    }
  }

  return ret;
}
