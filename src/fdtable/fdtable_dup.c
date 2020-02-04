#include "../fd.h"
#include "../fdstack.h"
#include "../fdtable.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* handles pending duplicating of a dup-(fd) initialized using fd_dup()
 *
 * it resets the D_OPEN flag of the (fd) if the operation
 * was successfully done
 *
 * values for flags:
 *
 *  FDTABLE_LAZY  do not flags
 *  FDTABLE_MOVE  flags dup()ing the file
 *  FDTABLE_FORCEPOS flags dup()ing the file to the specified fd
 *
 * returns -2 if still pending, -1 if failed, fd otherwise
 * ----------------------------------------------------------------------- */
int
fdtable_dup(struct fd* fd, int flags) {
  int state;
  int o;
  int e = -1;

  /* already resolved? */
  if((o = fd->e) == fd->n)
    return -1;

  /* if we can close the destination fd there's no need to wish,
     we'll be using dup2 in this case */
  if(flags & FDTABLE_CLOSE)
    state = fd->e;
  else
    state = fdtable_wish(fd->n, flags | FDTABLE_NOCLOSE);

  /* the wish may have (recursively) resolved our (fd) already */
  if(fd->e == fd->n)
    return FDTABLE_DONE;

retry:
  /* if the wish was satisfied or we should change the
     effective fd then dup() the file descriptor */
  if((fd->n == fd_exp) || (state == FDTABLE_DONE) || (flags & FDTABLE_MOVE))
    e = dup(o);

  /* position forced or destination fd can be closed */
  else if((state == fd->n) || (flags & FDTABLE_FORCE))
    e = dup2(o, fd->n);

  if(e == -1)
    return FDTABLE_ERROR;

  /* track the new file descriptor if its not above fd_exp */
  if(e <= fd_exp)
    fdtable_track(e, flags);

  /* if theres an fd in effective fd list at this position we
     have to remove it carefully by using fd_setfd first so
     it will not close the fd 'e' */
  if(fd_list[e]) {
    fd_setfd(fd_list[e], -1);
    fd_pop(fd_list[e]);
  }

  /* set the new effective fd */
  if(fd_ok(e))
    fd_setfd(fd, e);

  /* we didn't get the expected file descriptor and we're forcing, retry! */
  if(fd->e != fd->n && (flags & FDTABLE_FORCE)) {
    state = o;
    o = fd->e;
    fd->mode &= ~D_DUP;
    goto retry;
  }

  if(!(fd->mode & D_DUP))
    return o;

  fd->mode &= ~D_DUP;
  fd->dup = NULL;

  return FDTABLE_DONE;
}
