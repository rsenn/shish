#include "../sh.h"
#include "../eval.h"
#include "../fdstack.h"
#include "../vartab.h"

void
eval_exit(int exitcode) {
  struct eval *e, *parent_eval = sh->parent ? sh->parent->eval : 0;

  /* Walk the actual eval stack -- the global `eval` is the topmost push
     (see eval_push). sh->eval is never updated by eval_push (the line is
     commented out) so the previous walk over sh->eval was always NULL
     and `exit N` inside a subshell silently fell through to sh_exit's
     no-op path, leaving sh->exitcode at 0. autoconf's as_required probe
     `as_fn_failure () { as_fn_return 1; }` (where as_fn_return uses
     `(exit $1)`) depends on this propagating. */
  for(e = eval; e; e = e->parent) {
    if(e == parent_eval)
      return;

    if(e->jump && (e->flags & E_ROOT))
      break;
  }

  if(e) {
    if(e->destructor)
      exitcode = e->destructor(exitcode);

    e->exitcode = exitcode;

    /* Unwind the eval stack so the global `eval` matches `e` before we
       longjmp. The longjmp lands at e's setjmp site, which immediately
       calls eval_pop(e) -- that asserts e == eval. Without this loop,
       any intermediate evals pushed between e and the current top (e.g.
       a nested builtin_eval inside a subshell) are still on the list,
       eval_pop's assertion fires, and the shell aborts. fdstack and
       varstack still need to be unwound at the setjmp site, but the
       commented-out lines below would do that for THIS level only; the
       real unwind happens via the inner pops as they were skipped. */
    while(eval && eval != e)
      eval = eval->parent;

    /*while(fdstack != e->fdstack) fdstack_pop(fdstack);
    while(varstack != e->varstack) vartab_pop(varstack);*/

    longjmp(e->jumpbuf, (exitcode << 1) | 1);
  }
}
