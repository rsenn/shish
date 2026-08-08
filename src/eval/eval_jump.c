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

    /* a function call or subshell boundary blocks break/continue from
       reaching a loop outside it -- matching bash ("break: only
       meaningful in a `for', `while', or `until' loop", reported and
       otherwise ignored) rather than silently escaping through a
       function/subshell to whatever loop happens to enclose *that*,
       which the break/continue site doesn't actually lexically nest
       inside. Without this, every such escape also skipped every
       frame between the jump site and the (wrong, too-distant)
       target loop, since the same-shaped unwind loop below never got
       a chance to run for them -- leaking each one's env/vartab/
       fdstack state permanently, the same bug fixed for "return" in
       eval_return.c. Confirmed via a real crash: a long-running
       script whose subshells/functions do this enough times
       eventually corrupts sh->parent (a leaked struct env is stack-
       allocated -- once its own C stack frame is reused by later,
       unrelated calls, sh_forked()'s later walk over it reads
       whatever now occupies that memory) and crashes forking the
       next external command with a heap "double free or corruption"
       or straight SIGSEGV. */
    /* stop the search at the boundary, but don't discard a loop
       already matched *within* the current function/subshell/top-level
       scope -- POSIX/bash: "break N"/"continue N" with N greater than
       the actual nesting depth simply targets the outermost enclosing
       loop reachable without crossing such a boundary, it is not an
       error and does not silently no-op. Nulling "j" unconditionally
       here conflated that ordinary "N is too large" case (a valid "j"
       already found, just fewer than N loops available) with the real
       escape case this check exists to prevent (no loop matched *yet*
       when the boundary is hit, e.g. "break" called directly inside a
       function or subshell with no loop of its own) -- so "break 2"
       from a single enclosing loop at the very top of a script (whose
       own eval frame also carries E_ROOT, see sh_loop.c's E_ROOT
       tempflag) silently failed to break at all instead of breaking
       that one loop, exactly the way bash does. */
    if(e->flags & (E_FUNCTION | E_ROOT))
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
