#include "tree.h"
#include "expand.h"
#include "eval.h"
#include "var.h"

/* evaluate for-loop (3.9.4.2)
 * ----------------------------------------------------------------------- */
int eval_for(struct eval *e, struct nfor *nfor) {
  struct eval en;
  union node *node;
  union node *args = NULL;

  if(nfor->args)
    expand_args(nfor->args, &args, 0);

  node = args;

  eval_push(&en, 0);

  en.jump = 1;

  for(; node; node = node->list.next) {
    int jmpret;

    if((jmpret = setjmp(en.jmpbuf)) == 2)
      continue;
    else if(jmpret)
      break;

    /* iterate the loop variable */
    var_setvsa(nfor->varn, &node->narg.stra, V_DEFAULT);

    /* evaluate loop body */
    eval_tree(e, nfor->cmds, E_LIST);
  }

  if(args)
    tree_free(args);

  return eval_pop(&en);
}


