#include "../fd.h"
#include "../fdstack.h"
#include "../fdtable.h"
#include "../debug.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#include <fcntl.h>
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
  int state, o, e = -1;
  int retried = 0;

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
     effective d then dup() the file descriptor.

     the dup() here is a bet that the kernel's lowest free fd equals
     fd_expected (== d->n), so dup() lands exactly on the target.
     fd_expected is only the shell's guess, though -- if a kernel fd
     is still open at that slot the bet is lost, and it stays lost on
     every subsequent attempt (each dup() only raises the lowest free
     fd further). so the bet may be tried at most once: on the forced
     retry below we go straight to dup2(), which cannot miss. without
     this, a stale fd_expected made the retry loop dup() its own
     result forever, leaking one fd per iteration until EMFILE. */
  if(!retried && ((d->n == fd_expected) || (state == FDTABLE_DONE) || (flags & FDTABLE_MOVE)))
    e = dup(o);

  /* position forced or destination d can be closed */
  else if((state == d->n) || (flags & FDTABLE_FORCE))
    e = dup2(o, d->n);

  if(e == -1)
    return FDTABLE_ERROR;

#if !WINDOWS_NATIVE && defined(FD_CLOEXEC)
  /* Mark shell-internal bookkeeping fds close-on-exec so they don't leak
     into spawned external programs. Standard fds 0/1/2 are user-facing and
     must stay inheritable; anything else is the shell's private state. */
  if(e > STDERR_FILENO)
    fcntl(e, F_SETFD, FD_CLOEXEC);
#endif

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
     it will not close the d 'e'. Capture the pointer first:
     fd_setfd now clears fd_list[old_e] when it points at the struct
     being moved (the fd_list invariant fix), so re-reading fd_list[e]
     after fd_setfd would yield NULL and fd_pop would deref it. */
  {
    struct fd* victim = fd_list[e];
    if(victim) {
      fd_setfd(victim, -1);
      fd_pop(victim);
    }
  }

  /* set the new effective d */
  if(fd_ok(e))
    fd_setfd(d, e);

  /* we didn't get the expected file descriptor and we're forcing, retry!
     the missed dup() above created a stepping-stone fd we own
     exclusively: close it right away and retry with dup2() from the
     original o, so the ownership contract of the return value below
     stays intact. shifting o to the stepping stone instead (as this
     used to) meant nobody ever closed the original kernel fd -- the
     leaked fd then occupied a slot the table considers free, which is
     what made fd_expected go stale (and leaked fds into exec'd
     programs, since only the stepping stone is marked cloexec). */
  if(d->e != d->n && (flags & FDTABLE_FORCE)) {
    if(e >= 0 && e != o) {
      if(fd_ok(e) && fd_list[e] == d)
        fd_list[e] = 0;
      close(e);
    }
    retried = 1;
    goto retry;
  }

  if(!(d->mode & FD_DUP))
    return o;

  d->mode &= ~FD_DUP;
  d->dup = NULL;

  return FDTABLE_DONE;
}
