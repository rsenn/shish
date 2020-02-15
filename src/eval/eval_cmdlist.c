#include "../tree.h"
#include "../eval.h"

/* ----------------------------------------------------------------------- */
int
eval_cmdlist(struct eval* e, struct ngrp* grp) {
  int ret = 0;
  union node* cmd;

  for(cmd = grp->cmds; cmd; cmd = cmd->ncmd.next) {
    ret = eval_node(e, cmd);
  }

  return ret;
}
