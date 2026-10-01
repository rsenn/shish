#include "../eval.h"
#include "../tree.h"

struct eval_args* eval_args_top = NULL;

/* free the argument lists of the simple commands a longjmp is skipping.
 *
 *   struct eval_args*  to  innermost guard to keep (the target frame's)
 * ----------------------------------------------------------------------- */
void
eval_args_unwind(struct eval_args* to) {
  while(eval_args_top && eval_args_top != to) {
    tree_free(eval_args_top->args);
    eval_args_top = eval_args_top->prev;
  }
}
