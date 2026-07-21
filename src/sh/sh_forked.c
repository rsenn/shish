#include "../../lib/byte.h"
#include "../eval.h"
#include "../sh.h"
#include "../../lib/shell.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

int sh_child = 0;

/* after forking, returns pid
 * ----------------------------------------------------------------------- */
int
sh_forked(void) {
  struct env* e = sh;
  struct env* next;

  /* if we're not in the root environment we clean up any shell env */
  for(sh = sh->parent; sh; sh = next) {
    next = sh->parent;

    sh_setargs(NULL, 0);

    if(sh->cwd.a)
      stralloc_free(&sh->cwd);
    else
      sh->cwd.s = NULL;
  }

  sh = &sh_root;
  byte_copy(sh, sizeof(struct env), e);
  sh->parent = NULL;

  /* the global `eval` chain (virtual subshell/cmdsubst nesting, each
     frame possibly jump-enabled via setjmp) is inherited as-is across
     fork() -- its jumpbufs are still physically valid stack addresses
     in our copy of the stack, so eval_exit() would happily longjmp
     back into them. But those frames belong to control flow that, from
     this forked child's point of view, doesn't exist: we're a new
     process, and "exiting a subshell" here must always mean really
     terminating, never resuming some inherited caller's loop. Confirmed
     without this: a pipeline forked from inside a "(...)" subshell,
     e.g. "(true | true)", had its per-member forked children survive
     past their own sh_exit() by longjmping back into eval_subshell()'s
     setjmp, then falling through to interpret the rest of the script
     themselves -- each surviving fork became an extra, fully-running
     copy of the interpreter. */
  {
    struct eval* ev;

    for(ev = eval; ev; ev = ev->parent)
      ev->jump = 0;
  }

  sh_child = 1;

  /* sh_pid must reflect *this* (forked child) process's own pid from
     here on -- job_fork()'s child branch uses it to setpgid() itself
     into the right process group, and everything else that reports
     "my pid" (e.g. "$$") relies on it too. Left at the parent's pid
     (this line was previously commented out, doing nothing), the
     child's setpgid() call target and requested pgid were both wrong,
     which is why job_wait()'s wait4(-j->pgrp, ...) reliably failed
     with ECHILD instead of ever finding a backgrounded child: the
     child never actually ended up in the process group the parent
     was told to expect it in. */
  sh_pid = getpid();

  return sh_pid;
}
