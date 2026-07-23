#include "../eval.h"
#include "../sh.h"
#include "../source.h"
#include "../debug.h"
#include "builtin_config.h"

int trap_exit(int);

/* exits current subshell, never returns!
 * ----------------------------------------------------------------------- */
void
sh_exit(int retcode) {
  struct env* s = sh;

  /* if we're in a subshell, this jumps back where it was established
     and never returns -- eval_exit() itself runs the subshell's own
     EXIT trap (via its destructor callback) before jumping, so the
     unconditional trap_exit() call below must NOT also fire in that
     case, or the trap runs twice (once here with whatever EXIT trap
     happens to be globally installed at this point, once more via
     eval_exit()'s destructor with the correct, subshell-local one).
     trap_exit() below is reached only when eval_exit() found no
     subshell to jump back to, i.e. this really is the outermost
     shell exiting, which is the one case eval_exit() can't handle
     itself (its search requires a jump-enabled frame). */
  eval_exit(retcode);

#if BUILTIN_TRAP
  trap_exit(retcode);
#endif

  while(s->eval && s->eval->flags & E_FUNCTION)
    s = s->parent;

#ifdef DEBUG_ALLOC
  debug_memory();
#endif

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
