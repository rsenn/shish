#include "../eval.h"
#include "../sh.h"
#include "../source.h"
#include "builtin_config.h"

int trap_exit(int);

int sh_async_exit = 0;

/* exits current subshell, never returns!
 * ----------------------------------------------------------------------- */
void
sh_exit(int retcode) {
  struct env* s = sh;

  /* if we're in a subshell, this jumps back where it was established
     and never returns -- eval_exit() already runs the subshell's own
     EXIT trap via its destructor callback, so the trap_exit() call
     below must only fire when there was no subshell to jump back to
     (the outermost shell exiting), or the trap runs twice. */
  eval_exit(retcode);

#if BUILTIN_TRAP
  /* sh_child (sh_forked.c) marks a process that was fork()'d off to
     run one job/pipeline-stage and will simply _exit() once this
     call returns -- not the real shell exiting, so the parent's EXIT
     trap must not run here too (it still runs, once, in the actual
     top-level shell when its own sh_exit() call reaches this point). */
  if(!sh_child)
    trap_exit(retcode);
#endif

  while(s->eval && s->eval->flags & E_FUNCTION)
    s = s->parent;

  /* not in a subshell, exit the process. Only free cwd on the way out --
     the previous unconditional free here trashed sh->cwd on every fall-
     through (when eval_exit had nothing to longjmp to), so any subsequent
     sh_pop would dereference a freed buffer. */
  if(s == &sh_root) {
    stralloc_free(&sh->cwd);

    if(source)
      source_pop();

    exit(retcode);
  }
}
