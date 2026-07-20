#include "../fd.h"
#include "../sh.h"
#include "../eval.h"
#include "../fdstack.h"
#include "../source.h"
#include "../vartab.h"

void
eval_jump(int levels, int cont) {
  struct eval* e;
  struct eval* j = NULL;

  for(e = eval; e; e = e->parent) {
    if(levels <= 0)
      break;

    if(e->jump && (e->flags & E_LOOP)) {
      j = e;
      levels--;
    }
  }

  if(j) {
    eval = j;

    while(fdstack != j->fdstack)
      fdstack_pop(fdstack);

    while(varstack != j->varstack)
      vartab_pop(varstack);

    /* break/continue longjmps past any source_popfd() calls between here
       and the target loop frame -- e.g. "eval continue" leaves the eval
       builtin's own pushed source (its string-buffer fd) still on top of
       the global source stack, which corrupts every subsequent parse and
       hangs sh_loop(). eval_pop() already does this same recovery for
       the analogous "exit inside eval" case; mirror it here since this
       path bypasses eval_pop() for every frame between jump and target.
       Each popped frame may also own an fd (source_popfd() = source_pop()
       + fd_pop()) that's still linked into the current fdstack level's
       fd list -- fdstack_pop() above only unwinds whole levels, so
       without this that fd is left dangling, pointing at a stack frame
       the longjmp already unwound past. */
    while(source && source != j->source) {
      struct fd* f = source->fd;
      source_pop();

      if(f)
        fd_pop(f);
    }

    longjmp(j->jumpbuf, cont << 1);
  }
}
