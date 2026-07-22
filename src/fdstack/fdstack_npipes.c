#include "../fdstack.h"

/* returns how many pipes we have to establish from fdstack to fdstack->parent
 * supply FD_SUBST, FD_HERE or both of them
 * ----------------------------------------------------------------------- */
unsigned int
fdstack_npipes(int mode) {
  struct fd* fd;
  struct fdstack* st;
  unsigned int ret = 0;

  /* Walk outward through fdstack levels that don't themselves carry
     any FD_SUBST/FD_HERE target (redirection scopes, function-call
     scopes, ... all push their own empty level), but stop at the
     first level that does. A command substitution nested inside
     another one (e.g. "$(cmd $(cmd | cmd))") pushes its own such
     level for its own fd_subst() target; walking past that (into the
     OUTER substitution's still-pending target further out) would
     make fdstack_pipe() wire a pipe for the outer one too and hand
     it to whatever gets forked next here, instead of leaving it for
     the outer command substitution's own eval_tree()/eval_pipeline()
     call to handle when *it* forks.

     A plain redirection duplicating an already-FD_SUBST/FD_HERE fd
     (e.g. "2>&1" duplicating a command substitution's fd 1 target)
     also carries the same FD_SUBST/FD_HERE bits -- fd_dup() copies
     dupe->mode's FD_TYPE bits, which includes FD_STRALLOC -- onto its
     own FD_DUP'd struct at the *command's* own (inner) redirection
     scope. That's not a nested substitution's own target, just an
     alias of the outer one; counting it here stopped the walk one
     level too early, before ever reaching the outer target itself,
     which then never got a pipe wired for it at all (its output fell
     back to whatever real fd the shell's own stdout happened to be).
     Skip FD_DUP'd entries so the walk correctly continues past a
     same-target duplicate to the level that actually owns it. */
  for(st = fdstack; st; st = st->parent) {
    unsigned int here = 0;

    for(fd = st->list; fd; fd = fd->next)
      if(!(fd->mode & FD_DUP) &&
         ((fd->mode & mode) == FD_SUBST || (fd->mode & mode) == FD_HERE))
        here++;

    ret += here;

    if(here)
      break;
  }

  return ret;
}
