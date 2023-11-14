#include "../fd.h"
#include "../fdstack.h"
#include "../fdtable.h"
#include "../debug.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* handles pending duplicating of a dup-(fd) initialized using fd_dup()
 *
 * it resets the FD_OPEN flag of the (fd) if the operation
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
fdtable_dup(struct fd* d, int flags) {
  int state,o, e = -1;

  /* already resolved? */
  if((o = d->e) == d->n)
    return -1;

  /* if we can close the destination d there's no need to wish,
     we'll be using dup2 in this case */
  if(flags & FDTABLE_CLOSE)
    state = d->e;
  else
    state = fdtable_wish(d->n, flags | FDTABLE_NOCLOSE);

  /* the wish may have (recursively) resolved our (d) already */
  if(d->e == d->n)
    return FDTABLE_DONE;

retry:
  /* if the wish was satisfied or we should change the
     effective d then dup() the file descriptor */
  if((d->n == fd_expected) || (state == FDTABLE_DONE) || (flags & FDTABLE_MOVE))
    e = dup(o);

  /* position forced or destination d can be closed */
  else if((state == d->n) || (flags & FDTABLE_FORCE))
    e = dup2(o, d->n);

  if(e == -1)
    return FDTABLE_ERROR;

#if defined(DEBUG_OUTPUT) && defined(DEBUG_FDTABLE)
  buffer_puts(debug_output, COLOR_YELLOW "fdtable_dup" COLOR_NONE " #");
  buffer_putulong(debug_output, o);
  buffer_puts(debug_output, " = ");
  buffer_putulong(debug_output, e);
  debug_nl_fl();
#endif

  /* track the new file descriptor if its not above fd_expected */
  if(e <= fd_expected)
    fdtable_track(e, flags);

  /* if theres an d in effective d list at this position we
     have to remove it carefully by using fd_setfd first so
     it will not close the d 'e' */
  if(fd_list[e]) {
    fd_setfd(fd_list[e], -1);
    fd_pop(fd_list[e]);
  }

  /* set the new effective d */
  if(fd_ok(e))
    fd_setfd(d, e);

  /* we didn't get the expected file descriptor and we're forcing, retry! */
  if(d->e != d->n && (flags & FDTABLE_FORCE)) {
    state = o;
    o = d->e;
    d->mode &= ~FD_DUP;
    goto retry;
  }

  if(!(d->mode & FD_DUP))
    return o;

  d->mode &= ~FD_DUP;
  d->dup = NULL;

  return FDTABLE_DONE;
}
