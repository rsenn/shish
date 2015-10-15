#include "tree.h"
#include "eval.h"

/* evaluate a AND-OR list (3.9.3)
 * ----------------------------------------------------------------------- */
int eval_and_or(struct eval *e, struct nandor *nandor) {
  int ret;

  ret = eval_tree(e, nandor->cmd0, 0);

  if((nandor->id == N_AND && !ret) ||
      (nandor->id == N_OR && ret))
    ret = eval_tree(e, nandor->cmd1, 0);

  if(nandor->id == N_NOT)
    ret = !ret;

  return ret;
}


