#include "fdtable.h"
#include "fd.h"

/* handles closing of fd e maybe by resolving another fd
 * ----------------------------------------------------------------------- */
int fdtable_close(int e, int flags) {
  int state;

  /* don't waste the chance to resolve the fd above the current
     expected file descriptor, so go into a recursion */
  while(e < fd_exp && fdtable[fd_exp]) {
    state = fdtable_resolve(fdtable[fd_exp], flags);

    if(state == FDTABLE_ERROR)
      return FDTABLE_ERROR;

    if(state != FDTABLE_DONE)
      break;
  }

  /* set the close flag, because maybe we can
     resolve some other fd by closing this one */
  flags |= FDTABLE_CLOSE;

  /* do not recurse on NOCLOSE, which means
     we're already coming from fdtable_resolve */
  if((flags & FDTABLE_NOCLOSE) == 0) {
    if(fdtable[e])
      return fdtable_resolve(fdtable[e], flags);

    close(e);
  }

  if(fd_exp > e)
    fd_exp = e;

  return FDTABLE_DONE;
}
