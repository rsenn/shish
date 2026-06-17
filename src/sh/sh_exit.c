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

#if BUILTIN_TRAP
  trap_exit(retcode);
#endif

  /* we're in a subshell, jump back where we established it */
  eval_exit(retcode);

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
