#include "../fd.h"
#include "../fdstack.h"
#include "../fdtable.h"

/* request a gap below fd_exp, a gap can be created if the fd
 * is owned by an fd not on top. a gap can also be forced through
 * fd_resolve.
 *
 * fd is assumed to be smaller than 'fd_exp'
 *
 * return 1 if we made a gap
 * ----------------------------------------------------------------------- */
int
fdtable_gap(int e, int flags) {
  struct fd* gap;

  /* there is already a gap? */
  if((gap = fd_list[e]) == NULL)
    return FDTABLE_DONE;

  /* if we can close the gap fd then delete it */
  if((gap->mode & D_CLOSE) || ((flags & FDTABLE_FORCE) && gap != fdtable[gap->n])) {
    /* set fd to -1 so it isn't closed in fd_close()
       because we'll maybe handle this later by dup2() */
    if(flags & FDTABLE_NOCLOSE) {
      fd_setfd(gap, -1);
      fd_pop(gap);
      return e;
    }

    fd_pop(gap);
    return FDTABLE_DONE;
  }

  if(flags & FDTABLE_FORCE) {
    /* resolve the gap fd, but do not force
       position to prevent from infinite recursion */
    flags &= ~FDTABLE_FORCE;
    flags |= FDTABLE_MOVE;

    return fdtable_resolve(gap, flags);
  }

  return FDTABLE_PENDING;
}
