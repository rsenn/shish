#include "../fd.h"
#include "../fdtable.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* try to resolve the virtual to its effective file descriptor
 *
 * FDTABLE_LAZY   do only if we have a nice chance to
 * FDTABLE_MOVE   forces the fd->e to be (re-)mapped, preferrably to fd->n
 * FDTABLE_FORCE  forces the fd->e to be (re-)mapped to fd->n
 * FDTABLE_CLOSE  means that the current fd->e can already be closed
 * ----------------------------------------------------------------------- */
int
fdtable_resolve(struct fd* fd, int flags) {
  int state = FDTABLE_PENDING;

  /* already resolved */
  if(fd->e == fd->n)
    return FDTABLE_DONE;

  /* do not open/close if we don't need an effective fd */
  if((flags & FDTABLE_FD) == FDTABLE_LAZY) {
    if(fd->mode & (D_OPEN | D_CLOSE | D_STRALLOC))
      return state;
  }

  if((flags & FDTABLE_FD) && fd != fdtable[fd->n]) {
    fd->mode = D_CLOSE;
  }
  /*    state = fdtable_close(fd->n, flags);*/

  /* do some actions to resolve the effective file descriptor */
  switch(fd->mode & (D_OPEN | D_CLOSE | D_STRALLOC)) {
    /* we're forced to close */
    case D_CLOSE: {
      state = fdtable_close(fd->n, flags);
      break;
    }

      /* if the fd is still to be opened then try that */
    case D_OPEN: {
      state = fdtable_open(fd, flags);
      break;
    }

      /* drop here-docs to temp files */
    case D_STRALLOC: {
      if(D_ISRD(fd))
        state = fdtable_here(fd, flags);
      break;
    }
  }

  /* if we're not done yet we have to force the effective fd number */
  if(state != FDTABLE_DONE) {
    if(fd_ok(state))
      flags |= FDTABLE_CLOSE;

    /* try to move/duplicate the old effective file descriptor,
       maybe this returns a valid fd number which we'll have to
       close */
    state = fdtable_dup(fd, flags);

    if(fd_ok(state)) {
      /* if this is gonna change the expected fd we do a lazy
         check before */
      if(fd_exp < state) {
        if(fdtable_lazy(-1, flags) == FDTABLE_ERROR)
          return FDTABLE_ERROR;
      }

      close(state);
      state = FDTABLE_DONE;
    }
  }

  return state;
}
