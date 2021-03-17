#include "../fd.h"
#include "../sh.h"
#include "../eval.h"
#include "../tree.h"

/* evaluate if-conditional (3.9.4.4)
 * ----------------------------------------------------------------------- */
int
eval_if(struct eval* e, struct nif* nif) {
  int ret;
  union node* branch;

elif:
  ret = eval_tree(e, nif->test, E_LIST);

  /* do not recurse for elifs */
  if(ret && nif->cmd1) {
    if(nif->cmd1->nif.id == N_IF) {
      nif = &nif->cmd1->nif;
      goto elif;
    }
  }

  /* take the branch */
  branch = ret ? nif->cmd1 : nif->cmd0;

  if(branch)
    return eval_tree(e, branch, E_LIST);

  return 0;
}
