#include "../expand.h"
#include "../tree.h"
#include "../var.h"
#include "../../lib/uint64.h"
#include "../../lib/scan.h"
#include <assert.h>

size_t scan_number(const char* x, int64* n, unsigned* base);

/* expand a assignment expression
 * ----------------------------------------------------------------------- */
int
expand_arith_assign(struct narithbinary* assign, int64* r) {
  int64 left = 0, right = 0;
  char buf[FMT_LONG * 2];

  const char *s, *name = assign->left->nargparam.name;
  assert(assign->left->id == N_ARGPARAM);

  if((s = var_vdefault(name, "0", 0)) == 0 || !scan_number(s, &left, 0))
    left = 0;

  if(expand_arith_expr(assign->right, &right))
    right = 0;

  switch(assign->id) {
    case A_VASSIGN: left = right; break;
    case A_VADD: left += right; break;
    case A_VSUB: left -= right; break;
    case A_VMUL: left *= right; break;
    case A_VDIV: left /= right; break;
    case A_VMOD: left %= right; break;
    case A_VSHL: left <<= right; break;
    case A_VSHR: left >>= right; break;
    case A_VBITAND: left &= right; break;
    case A_VBITXOR: left ^= right; break;
    case A_VBITOR: left |= right; break;
    default: return 1;
  }

  var_setv(name, buf, fmt_longlong(buf, left), V_DEFAULT);

  *r = left;
  return 0;
}
