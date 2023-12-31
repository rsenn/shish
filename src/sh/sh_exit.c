#include "../eval.h"
#include "../sh.h"
#include "../source.h"
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

  /* not in a subshell, exit the process */
  if(s == &sh_root) {
    if(source)
      source_pop();

    exit(retcode);
  }
}
