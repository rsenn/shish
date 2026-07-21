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

  return 0; /*(sh_pid = getpid());*/
}
