#include "../tree.h"
#include "../eval.h"
#include "../sh.h"

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

    /* "set -e": every ";"/newline-separated command on the same input
       line as this one is represented as an N_LIST wrapper around this
       loop rather than eval_tree()'s own per-node chain -- without a
       matching check here, "set -e; false; echo bad" (all one line,
       one N_LIST node) never noticed "false" failing at all, only
       "set -e; false" on separate lines (each its own top-level
       eval_tree() call) did. See eval_tree.c's matching check (and
       errexit_suppress's own comment, eval.h) for the full reasoning
       behind every one of these exclusions -- it's word for word the
       same list, for word for word the same reasons. Note this whole
       list is naturally exempt in its own right, with no special
       handling needed here beyond checking errexit_suppress like
       everyone else: an if/while/until condition list ("if false;
       true; then") reaches here with errexit_suppress already
       incremented by eval_if.c/eval_loop.c around the *entire*
       eval_tree() call that dispatched to this eval_cmdlist(), so
       every member of this loop sees it suppressed, not just the
       first one that happens to be N_LIST-wrapped. */
    if(sh->opts.errexit && ret != 0 && !errexit_suppress && cmd->id != N_NOT &&
       cmd->id != N_AND && cmd->id != N_OR && cmd->id != N_BRACEGROUP && cmd->id != N_IF &&
       cmd->id != N_FOR && cmd->id != N_CASE && cmd->id != N_WHILE && cmd->id != N_UNTIL &&
       cmd->id != N_LIST)
      sh_exit(ret);
  }

  return ret;
}
