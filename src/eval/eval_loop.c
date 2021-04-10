#include "../eval.h"
#include "../tree.h"
#include "../sh.h"

/* evaluate while/until-loop (3.9.4.5 - 3.9.4.6)
 * ----------------------------------------------------------------------- */
int
eval_loop(struct eval* e, struct nloop* nloop) {
  struct eval en;
  int retcode = (nloop->id == N_WHILE) ? 0 : 1;
  int ret = 0;

  eval_push(&en, E_LOOP);

  en.jump = 1;
  if(setjmp(en.jumpbuf) & 1)
    return ret;

  while((ret = eval_tree(e, nloop->test, E_LIST) == retcode)) {
    /* evaluate loop body */
    eval_tree(e, nloop->cmds, E_LIST);
  }

  return eval_pop(&en);
}
