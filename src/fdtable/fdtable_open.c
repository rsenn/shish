#include "fd.h"
#include "fdtable.h"
#include "sh.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* handles pending opening of a file-fd initialized using fd_open()
 *
 * it resets the D_OPEN flag of the fd if the operation
 * was successfully done
 *
 * values for flags:
 *
 * FDTABLE_LAZY     do not force
 * FDTABLE_CLOSE    close fd before opening the file
 * FDTABLE_MOVE     force opening the file
 * FDTABLE_FORCEPOS force opening the file at the specified fd
 *
 * returns fd or -2 if not yet done, -1 if failed,
 * otherwise the new effective file descriptor
 * ----------------------------------------------------------------------- */
int
fdtable_open(struct fd* fd, int flags) {
  int e;
  int state;

  /* not a pending open()? */
  if((fd->mode & D_OPEN) == 0)
    return FDTABLE_DONE;

  /* wish fd->n become the next expected file descriptor */
  state = fdtable_wish(fd->n, flags);

  /* the wish may have (recursively) resolved our fd already */
  if(fd->e == fd->n)
    return FDTABLE_DONE;

  /* leave it for now if we're in lazy mode and still pending */
  if(state == FDTABLE_PENDING && (flags & FDTABLE_FD) == FDTABLE_LAZY)
    return state;

  /* maybe we can close the destination fd */
  if(state == fd->n)
    close(fd->n);

  /* now open the file, and return -1 if we failed */
  e = open(fd->name, fd->fl, (0666 & ~sh->umask));

  if(!fd_ok(e)) {
    sh_error(fd->name);
    return FDTABLE_ERROR;
  }

  /* track new bottom file descriptor and set the new fd */
  fdtable_track(e, flags);
  fd_setfd(fd, e);

  fd->mode &= ~D_OPEN;

  if(fd->n != e) {
    /* we should have been done, but doesn't look like,
       let fdtable_dup in fdtable_resolve do the work */
    if(state >= FDTABLE_DONE)
      return fd->n;

    return FDTABLE_PENDING;
  }

  return FDTABLE_DONE;
}
