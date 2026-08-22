#include "../fd.h"
#include "../sh.h"
#include "../eval.h"
#include "../fdstack.h"
#include "../source.h"
#include "../vartab.h"

void
eval_return(int value) {
  struct eval *e, *f = NULL;

  /* stop at the nearest E_FUNCTION frame *or* the nearest E_ROOT one
     (a subshell -- eval_subshell.c -- or a command substitution --
     expand_command.c), whichever comes first walking outward.
     POSIX/bash treat "return" inside a subshell the same as "exit"
     for that subshell: it terminates the subshell with that status
     and does not propagate to whatever function the subshell itself
     happens to be nested in. eval_exit() already searches for
     E_ROOT for exactly this reason (see its own comment); searching
     only for E_FUNCTION here missed the subshell case entirely --
     "f() { (return 5); echo after; }; f" jumped straight out through
     f itself, silently skipping the subshell boundary, "echo after"
     never running, and f's own exit status coming out as 5 instead
     of whatever ran after the subshell. */
  for(e = eval; e; e = e->parent) {
    if(e->jump && (e->flags & (E_FUNCTION | E_ROOT))) {
      f = e;
      break;
    }
  }

  if(f) {
    if(f->destructor)
      value = f->destructor(value);

    eval = f;

    /* mirror eval_jump()'s unwind (break/continue): "return" finds
       the nearest E_FUNCTION frame, which may not be the *immediately*
       enclosing one -- a subshell (E_ROOT), a command substitution, or
       the "eval" builtin can all sit between the return site and it
       without matching E_FUNCTION themselves, so the longjmp below
       would otherwise skip past each one's vartab_pop()/fdstack_pop()
       (and, for a real subshell, eval_subshell()'s own sh_pop())
       entirely. Previously this was commented out, which both leaked
       every such frame's env/vartab/fdstack state permanently (a
       confirmed unbounded leak: RSS under a tight loop of
       "f() { (return 5); }" grew linearly forever, ~370 bytes/call)
       and, since control never returns to the skipped frame's own
       code, let a bare "return" inside "(...)" propagate out through
       the *enclosing* function instead of just ending the subshell
       (eval-return-skips-frame-cleanup-and-leaks). */
    while(fdstack != f->fdstack)
      fdstack_pop(fdstack);

    while(varstack != f->varstack)
      vartab_pop(varstack);

    /* same source_popfd() recovery eval_jump()/eval_pop() already do
       for the analogous cases -- see eval_jump.c's comment. */
    while(source && source != f->source) {
      struct fd* fd = source->fd;
      source_pop();

      if(fd)
        fd_pop(fd);
    }

    longjmp(f->jumpbuf, value << 1);
  }
}
