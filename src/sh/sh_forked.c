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
  char** keep_argv = e->arg.v;

  /* if we're not in the root environment we clean up any shell env */
  for(sh = sh->parent; sh; sh = next) {
    next = sh->parent;

    /* an ancestor's positional-parameter array is only safe to free
       here if (e) -- flattened onto sh_root just below -- doesn't
       still reference the same array. sh_pushargs() has every
       non-owning nested env frame copy its parent's arg.v pointer
       verbatim (arg.a == 0 means "someone further up owns this"), so
       (e)'s own arg.v can be the same array as an ancestor's. */
    if(sh->arg.v != keep_argv)
      sh_setargs(NULL, 0);

    if(sh->cwd.a)
      stralloc_free(&sh->cwd);
    else
      sh->cwd.s = NULL;
  }

  sh = &sh_root;
  byte_copy(sh, sizeof(struct env), e);
  sh->parent = NULL;

  /* sh->eval was copied verbatim from whatever env was active at fork
     time; if that was a shell function call, the copy carries
     E_FUNCTION along with it. sh_exit()'s unwind loop then walks
     sh->parent looking for a non-E_FUNCTION frame, but sh->parent is
     NULL (we're the root now), so it would crash. Clear the flag: a
     freshly forked process has no function call left to unwind out
     of. */
  if(sh->eval)
    sh->eval->flags &= ~E_FUNCTION;

  /* the global `eval` chain (virtual subshell/cmdsubst nesting, each
     frame possibly jump-enabled via setjmp) is inherited as-is across
     fork() -- its jumpbufs are still valid stack addresses in our copy
     of the stack, so eval_exit() would happily longjmp back into them.
     But from this child's point of view those frames don't exist: a
     new process must always really terminate here, never resume some
     inherited caller's loop. */
  {
    struct eval* ev;

    for(ev = eval; ev; ev = ev->parent)
      ev->jump = 0;
  }

  sh_child = 1;

  /* sh_pid must reflect this child process's own pid from here on --
     job_fork()'s child branch uses it to setpgid() itself into the
     right process group, and "$$" relies on it too. */
  sh_pid = getpid();

  return sh_pid;
}
