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

  /* E_EXIT ("tail command, exec instead of forking, see eval_tree.c")
     bled down from our own caller applies to the branch actually
     taken, once chosen -- never to the test, which isn't in tail
     position no matter what runs after this if-statement. Without
     masking it off first, eval_tree(e, nif->test, ...)'s own copy of
     this same "ex" logic saw it too and exited the whole process
     right after the test ran, before the branch below ever got a
     chance to (pipeline-compound-commands-broken: "echo x | if
     true; then cat; fi" produced no output, since eval_pipeline.c
     forks each stage and hands its own eval_tree() call E_EXIT). */
  int ex = (e->flags & E_EXIT) != 0;

elif:
  /* the controlling list of an if/elif is exempt from "set -e" in its
     entirety -- see errexit_suppress's own comment (eval.h). */
  errexit_suppress++;
  e->flags &= ~E_EXIT;
  ret = eval_tree(e, nif->test, E_LIST);
  errexit_suppress--;

  /* do not recurse for elifs */
  if(ret && nif->cmd1) {
    if(nif->cmd1->nif.id == N_IF) {
      nif = &nif->cmd1->nif;
      goto elif;
    }
  }

  /* take the branch */
  branch = ret ? nif->cmd1 : nif->cmd0;

  if(branch) {
    if(ex)
      e->flags |= E_EXIT;

    return eval_tree(e, branch, E_LIST);
  }

  return 0;
}
