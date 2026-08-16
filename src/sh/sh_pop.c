#include "../eval.h"
#include "../fdstack.h"
#include "../sh.h"
#include "../vartab.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
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

  /* change back to prev working dir (skip if our cwd was already freed,
     e.g. by sh_exit's fall-through path) */
  if(sh->cwd.s && stralloc_diffs(&sh->cwd, parent->cwd.s)) {
    if(chdir(parent->cwd.s) == -1)
      sh_errorn_errno(parent->cwd.s, parent->cwd.len);
  }

  /* same idea as the cwd restore above, for the process umask: a
     subshell's "umask NNN" updates sh->umask for its own struct env,
     but only builtin_umask.c calls the real umask() syscall -- so the
     process-wide umask must be explicitly restored here too. */
  if(sh->umask != parent->umask)
    umask(parent->umask);

  /* free arguments */
  sh_setargs(NULL, 0);

  /* free current env and pop the parent */
  if(sh->cwd.a)
    stralloc_free(&sh->cwd);

  /*while(sh->eval)
    eval_pop(sh->eval);*/

  while(fdstack != sh->fdstack)
    fdstack_pop(fdstack);

  while(varstack != sh->varstack)
    vartab_pop(varstack);

  sh = parent;
  return 1;
}
