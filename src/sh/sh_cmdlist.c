#include "../sh.h"
#include "../eval.h"

/* execute a tree in a subshell
 *
 * you have to push stuff yourself!
 * ----------------------------------------------------------------------- */
int
sh_cmdlist(union node* cmd) {
  int ret;

  ret = eval_cmdlist(sh->eval, &cmd->ngrp);
  return ret;
}
