#include "../fd.h"
#include "../eval.h"
#include "../tree.h"
#include "../sh.h"

void trap_run_pending(void);

/* evaluate a tree node(-list maybe)
 * ----------------------------------------------------------------------- */
int errexit_suppress = 0;

int
eval_tree(struct eval* e, union node* node, int tempflags) {
  int ret = 0;
  int list = 0, ex = 0;
  // int oldflags;

  if((e->flags | tempflags) & E_LIST) {
    list = 1;
    e->flags &= ~E_LIST;
    tempflags &= ~E_LIST;
  }

  if((e->flags | tempflags) & E_EXIT) {
    ex = 1;
    e->flags &= ~E_EXIT;
    tempflags &= ~E_EXIT;
  }

  // oldflags = e->flags;
  e->flags |= tempflags;

  while(node) {
    /* a signal that arrived while the previous command ran is only
       recorded by trap_relay(); dispatch it here so a trap fires
       between two commands of the same list, not just at the line
       boundaries sh_loop()/term_read()/job_wait() cover. */
    trap_run_pending();

    /* not the last node, disable E_EXIT for now */
    if(ex && (!list || node->next == NULL))
      e->flags |= E_EXIT;

    /* eval_node_bgnd() forks a backgrounded compound command
       ("{ cmd; } &", "(cmd) &", ...) instead of running it in-process
       -- see its own comment for why a bare eval_node() call is wrong
       here */
    ret = eval_node_bgnd(e, node);
    e->exitcode = ret;

    /* "set -e": a failing command triggers it, with POSIX's specific
       exemptions -- errexit_suppress (eval.h) covers a "!"-negated
       pipeline/command (eval_and_or.c computes N_NOT's exit status by
       negating its single operand, so the *un*negated failure that
       made that negation meaningful must not itself be treated as
       this list's own failure) and an AND-OR list member other than
       the one that actually determined its result. Separately,
       N_AND/N_OR/N_NOT are *also* skipped here even when not
       suppressed, for a sharper reason than double-checking:
       eval_and_or() already applies this same check, correctly, to
       whichever operand actually ran last; re-checking its *returned*
       value here breaks the moment the list short-circuits, e.g.
       "false && true" -- confirmed directly against bash. The same
       reasoning extends to every other transparent compound construct
       that doesn't produce its own independent, opaque status
       (grouping, if/elif/else, for, case, while/until bodies, and a
       ";"-separated N_LIST sub-list) -- all of these already ran
       their own contents through this exact check (or
       eval_cmdlist()'s matching one) on the way here, so if nothing
       fired, either nothing failed, or what failed was itself exempt;
       re-checking the compound construct's own bubbled-up return
       value at this level would wrongly re-trigger on the latter case
       (confirmed directly against bash: "set -e; { false && true; };
       echo reached" prints "reached", but "set -e; (false && true);
       echo reached" does not -- a subshell's own status *is*
       independent/opaque and must still be checked normally, same as
       a simple command or pipeline).

       Ends the shell/subshell/function this eval frame belongs to,
       the same way an explicit "exit" would (sh_exit() already knows
       how to unwind exactly one level: the whole process at the top
       level, or just the innermost subshell/function otherwise). */
    if(sh->opts.errexit && ret != 0 && !errexit_suppress && node->id != N_NOT &&
       node->id != N_AND && node->id != N_OR && node->id != N_BRACEGROUP && node->id != N_IF &&
       node->id != N_FOR && node->id != N_CASE && node->id != N_WHILE && node->id != N_UNTIL &&
       node->id != N_LIST)
      sh_exit(ret);

    if(!list)
      break;

    node = node->next;
  }

  if(ex)
    sh_exit(ret);

  return ret;
}
