#include "../tree.h"
#include "../eval.h"

/* ----------------------------------------------------------------------- */
int
eval_cmdlist(struct eval* e, struct ngrp* grp) {
  int ret = 0;
  union node* cmd;

  /* eval_node_bgnd() (not a bare eval_node()) so a backgrounded
     compound command sharing this list with other commands
     ("{ cmd; } & echo after" -- more than one and-or-list on the same
     input line, chained here instead of eval_tree()'s own loop) still
     forks and returns immediately instead of running in-process; see
     eval_node_bgnd()'s comment for the full reasoning */
  for(cmd = grp->cmds; cmd; cmd = cmd->next) {
    ret = eval_node_bgnd(e, cmd);
  }

  return ret;
}
