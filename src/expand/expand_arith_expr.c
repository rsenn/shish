#include "../expand.h"
#include "../../lib/scan.h"
#include "../tree.h"
#include "../debug.h"
#include "../../lib/uint64.h"
#include <assert.h>

/* expand a binary expression
 * ----------------------------------------------------------------------- */
int
expand_arith_expr(union node* expr, int64* r) {
  int ret = 0;

  switch(expr->id) {
    case N_ARGCMD: {
      stralloc sa;
      stralloc_init(&sa);
      expand_tosa(expr, &sa);
      stralloc_nul(&sa);

      *r = 0;
      scan_longlong(sa.s, r);
      stralloc_free(&sa);
      break;
    }
    case A_PAREN: {
      ret = expand_arith_expr(((struct narithunary*)expr)->node, r);
      break;
    }

    case A_UNARYMINUS:
    case A_UNARYPLUS:
    case A_NOT:
    case A_BNOT:
    case A_PREINCR:
    case A_PREDECR:
    case A_POSTINCR:
    case A_POSTDECR: {
      ret = expand_arith_unary(&expr->narithunary, r);
      break;
    }
    case A_OR:
    case A_AND:
    case A_BITOR:
    case A_BITXOR:
    case A_BITAND:
    case A_EQ:
    case A_NE:
    case A_LT:
    case A_GT:
    case A_GE:
    case A_LE:
    case A_SHL:
    case A_SHR:
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
    case A_VASSIGN:
    case A_VADD:
    case A_VSUB:
    case A_VMUL:
    case A_VDIV:
    case A_VMOD:
    case A_VSHL:
    case A_VSHR:
    case A_VBITAND:
    case A_VBITXOR:
    case A_VBITOR: {
      ret = expand_arith_assign(&expr->narithbinary, r);
      break;
    }
    default: {
      debug_s("expand_arith_expr ");
      debug_node(expr, 1);
      debug_nl_fl();
      ret = -1;
      break;
    }
  }

  return ret;
}
