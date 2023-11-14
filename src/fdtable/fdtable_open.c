#include "../fd.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* handles pending opening of a file-fd initialized using fd_open()
 *
 * it resets the FD_OPEN flag of the fd if the operation
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
fdtable_open(struct fd* d, int flags) {
  int e;
  int state;

  /* not a pending open()? */
  if((d->mode & FD_OPEN) == 0)
    return FDTABLE_DONE;

  /* wish d->n become the next expected file descriptor */
  state = fdtable_wish(d->n, flags);

  /* the wish may have (recursively) resolved our d already */
  if(d->e == d->n)
    return FDTABLE_DONE;

  /* leave it for now if we're in lazy mode and still pending */
  if(state == FDTABLE_PENDING && (flags & FDTABLE_FD) == FDTABLE_LAZY)
    return state;

  /* maybe we can close the destination d */
  if(state == d->n)
    close(d->n);

  /* now open the file, and return -1 if we failed */
  e = open(d->name,
           d->fl /*| ((sh->opts.noclobber && (d->fl & O_CREAT)) ? O_EXCL : 0)*/,
           (0666 & ~sh->umask));

  if(!fd_ok(e)) {
    sh_error(d->name);
    return FDTABLE_ERROR;
  }

  /* track new bottom file descriptor and set the new d */
  fdtable_track(e, flags);
  fd_setfd(d, e);

  d->mode &= ~FD_OPEN;

  if(d->n != e) {
    /* we should have been done, but doesn't look like,
       let fdtable_dup in fdtable_resolve do the work */
    if(state >= FDTABLE_DONE)
      return d->n;

    return FDTABLE_PENDING;
  }

  return FDTABLE_DONE;
}
