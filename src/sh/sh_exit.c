#include "../eval.h"
#include "../sh.h"
#include "../source.h"

/* exits current subshell, never returns!
 * ----------------------------------------------------------------------- */
void
sh_exit(int retcode) {
  /* we're in a subshell, jump back where we established it */
  eval_exit(retcode);

  /* not in a subshell, exit the process */
  if(sh == &sh_root && source)
    source_pop();

  exit(retcode);
}
