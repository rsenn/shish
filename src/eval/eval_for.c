#include "../eval.h"
#include "../expand.h"
#include "../tree.h"
#include "../var.h"
#include "../fd.h"

/* evaluate for-loop (3.9.4.2)
 * ----------------------------------------------------------------------- */
int
eval_for(struct eval* e, struct nfor* nfor) {
  struct eval en;
  union node* node;
  union node* args = NULL;

  if(nfor->args)
    expand_args(nfor->args, &args, 0);

  eval_push(&en, 0);

  en.jump = 1;

  for(node = args; node; node = node->next) {
    int jmpret;

    if((jmpret = setjmp(en.jumpbuf)) == 2)
      continue;
    else if(jmpret)
      break;

    if(e->flags & E_PRINT) {
      eval_print_prefix(e, fd_err->w);
      buffer_putm_internal(fd_err->w, "for ", nfor->varn, " in ", 0);
      tree_print(nfor->args, fd_err->w);
      buffer_putnlflush(fd_err->w);
    }

    /* iterate the loop variable */
    var_setvsa(nfor->varn, &node->narg.stra, V_DEFAULT);

    /* evaluate loop body */
    eval_tree(e, nfor->cmds, E_LIST);
  }

  if(args)
    tree_free(args);

  return eval_pop(&en);
}
