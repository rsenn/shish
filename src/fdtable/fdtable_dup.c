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

/* handles pending duplication of a dup-fd initialized via fd_dup();
 * clears FD_OPEN on success.
 *
 * flags:
 *  FDTABLE_LAZY      do not flags
 *  FDTABLE_MOVE      flags dup()ing the file
 *  FDTABLE_FORCEPOS  flags dup()ing the file to the specified fd
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
  /* dup() is a bet that the kernel's lowest free fd equals fd_expected
   * (== d->n), landing exactly on the target. That bet can only be
   * tried once: a miss only raises the lowest free fd further, so a
   * forced retry goes straight to dup2() below, which cannot miss. */
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

  /* remove any fd already occupying slot e: fd_setfd first, so it
     won't close 'e'. Capture the pointer before calling fd_setfd,
     since fd_setfd clears fd_list[e] when it points at the struct
     being moved. */
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

  /* didn't get the expected fd and we're forcing: the missed dup()
   * above left a stepping-stone fd we own exclusively. Close it and
   * retry with dup2() from the original o. */
  if(d->e != d->n && (flags & FDTABLE_FORCE)) {
    if(e >= 0 && e != o) {
      if(fd_ok(e) && fd_list[e] == d)
        fd_list[e] = 0;
      close(e);
      fdtable_untrack(e);
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
