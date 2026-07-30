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
     subshell's "umask NNN" (e.g. autoconf/gnulib's routine
     "(umask 077 && mkdir ...)" idiom for a private tmpdir) correctly
     updates sh->umask for the subshell's own struct env, but that
     alone doesn't touch the real, process-wide umask() the kernel
     actually applies to every open()/mkdir() -- only builtin_umask.c
     calls the real umask() syscall, and only when *it* runs, not
     here. Without this, "$(umask)" lies (it just reads the correctly-
     popped-back sh->umask field) while every file/directory actually
     created afterward -- e.g. configure's own conftest.c -- silently
     keeps getting created with the subshell's more restrictive mode
     for the rest of the process's life, eventually manifesting as
     "conftest.c: Permission denied"/"C compiler cannot create
     executables" once something later needs to read or rewrite one
     of them and can't. */
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
