#include "eval.h"
#include "fdstack.h"
#include "sh.h"
#include "vartab.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* destroys current shell environment and pops previous
 * ----------------------------------------------------------------------- */
int
sh_pop(struct env* env) {
  struct env* parent;

  if(env != NULL && env != sh)
    return 0;

  if((parent = sh->parent) == NULL)
    return 0;

  /* change back to prev working dir */
  if(stralloc_diffs(&sh->cwd, parent->cwd.s)) {
    if(chdir(parent->cwd.s) == -1)
      sh_errorn(parent->cwd.s, parent->cwd.len);
  }

  /* free arguments */
  sh_setargs(NULL, 0);

  /* free current env and pop the parent */
  if(sh->cwd.a)
    stralloc_free(&sh->cwd);

  while(sh->eval) eval_pop(sh->eval);

  while(fdstack != sh->fdstack) fdstack_pop(fdstack);

  while(varstack != sh->varstack) vartab_pop(varstack);

  sh = parent;
  return 1;
}
