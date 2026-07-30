#include "../eval.h"
#include "../tree.h"

/* evaluate a AND-OR list (3.9.3)
 * ----------------------------------------------------------------------- */
int
eval_and_or(struct eval* e, struct nandor* nandor) {
  int ret;

  /* the left operand of an AND-OR list ("cmd1" in "cmd1 && cmd2" /
     "cmd1 || cmd2") -- and, since eval_and_or() computes N_NOT's own
     value by negating just this same call's result, a "!"-negated
     command's single operand too -- is exempt from "set -e" even if
     it fails; see errexit_suppress's own comment (eval.h). Only the
     right operand (the one case within an AND-OR list that's actually
     checked, when reached) is left at the ambient suppression level:
     if this whole and-or is itself just one operand of some *outer*
     and-or/condition -- e.g. the left "(false || false)" of "false ||
     false || true", which parses as "(false || false) || true" --
     errexit_suppress is already nonzero on entry here (the outer
     eval_and_or()'s own increment below is still in effect while its
     left operand, i.e. this whole call, runs), so leaving it alone
     (not resetting to 0) naturally propagates that exemption to both
     of *my* operands too, without this function needing to know or
     care that it's nested. Confirmed directly against bash: "set -e;
     false || false || true; echo reached" prints "reached" (the
     un-exempted middle "false" must not fire on its own). */
  errexit_suppress++;
  ret = eval_tree(e, nandor->left, 0);
  errexit_suppress--;

  if((nandor->id == N_AND && !ret) || (nandor->id == N_OR && ret))
    ret = eval_tree(e, nandor->right, 0);

  if(nandor->id == N_NOT)
    ret = !ret;

  return ret;
}
