#include "../tree.h"
#include "../eval.h"
#include "../sh.h"

/* ----------------------------------------------------------------------- */
int
eval_cmdlist(struct eval* e, struct ngrp* grp) {
  int ret = 0;
  union node* cmd;

  /* "exec the tail command directly instead of forking" (E_EXIT,
     eval_tree.c) must only ever apply to the *last* member of this
     list, exactly like eval_tree()'s own per-node loop already gets
     right -- but unlike eval_tree(), this function used to never
     touch e->flags's E_EXIT bit at all, so whatever the *caller* left
     it at (e.g. eval_pipeline.c's "exit(eval_tree(e, node, E_EXIT))",
     forking each pipeline stage and telling the last command in it to
     exec-and-never-return) stayed set for every single member of this
     loop, not just the last one. eval_simple_command.c reads it
     directly to decide whether to pass X_EXEC to exec_command() --
     so "{ echo reached1; false; echo reached2; } | cat" had "echo
     reached1" (the *first* member, not the last) treated as the tail
     call: it ran, then the forked pipeline child exited immediately,
     losing "false" and "echo reached2" entirely (confirmed as the
     actual bug behind BUGS:
     grouping-piped-loses-output-after-internal-failure -- and, since
     it reproduces with a "true" in place of "false" too, was never
     really about the internal failure at all). N_LIST (a plain
     ";"-separated sequence, also dispatched straight to this function
     by eval_node.c, not through eval_tree()) had the exact same
     exposure. */
  int ex = (e->flags & E_EXIT) != 0;

  e->flags &= ~E_EXIT;

  /* eval_node_bgnd() (not a bare eval_node()) so a backgrounded
     compound command sharing this list with other commands
     ("{ cmd; } & echo after" -- more than one and-or-list on the same
     input line, chained here instead of eval_tree()'s own loop) still
     forks and returns immediately instead of running in-process; see
     eval_node_bgnd()'s comment for the full reasoning */
  for(cmd = grp->cmds; cmd; cmd = cmd->next) {
    if(ex && cmd->next == NULL)
      e->flags |= E_EXIT;

    ret = eval_node_bgnd(e, cmd);
    e->flags &= ~E_EXIT;

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

  /* mirrors eval_tree()'s own trailing "if(ex) sh_exit(ret);" -- needed
     for the same reason: E_EXIT's caller (currently only
     eval_pipeline.c, wrapping its own eval_tree() call in an explicit
     exit()) may be relying on *something* along this call chain to
     actually terminate the process once the tail command's own
     exec_command() call returns normally instead of exec()ing (e.g.
     because it dispatched to a builtin, not an external program).
     Redundant with eval_pipeline.c's own wrapper today, but keeps this
     function's own contract for E_EXIT consistent with eval_tree()'s
     regardless of who ends up calling it. */
  if(ex)
    sh_exit(ret);

  return ret;
}
