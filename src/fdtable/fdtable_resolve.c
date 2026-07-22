#include "../fd.h"
#include "../fdtable.h"
#include "../debug.h"
#include "../sh.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* fdtable_resolve, fdtable_close, fdtable_dup, fdtable_here,
 * fdtable_open, fdtable_gap, fdtable_wish and fdtable_lazy recurse into
 * each other, following whatever fd currently occupies the slot a
 * redirection wants. That's normally a short, acyclic chain, but
 * nothing here detects a cycle (e.g. two or more fds all wanting each
 * other's slot) -- so a pathological redirection could in principle
 * recurse forever and blow the stack. There can be at most FDTABLE_SIZE
 * distinct fds involved, so any legitimate resolution finishes well
 * within that many nested calls; past it, we're going in circles.
 * ----------------------------------------------------------------------- */
static int
fdtable_resolve_depth;

/* try to resolve the virtual to its effective file descriptor
 *
 * FDTABLE_LAZY   do only if we have a nice chance to
 * FDTABLE_MOVE   forces the fd->e to be (re-)mapped, preferrably to fd->n
 * FDTABLE_FORCE  forces the fd->e to be (re-)mapped to fd->n
 * FDTABLE_CLOSE  means that the current fd->e can already be closed
 * ----------------------------------------------------------------------- */
static int
fdtable_resolve_1(struct fd* d, int flags) {
  int state = FDTABLE_PENDING;

  /* already resolved */
  if(d->e == d->n)
    return FDTABLE_DONE;

  /* an explicitly closed fd (">&-"/"<&-", see fd_null()): its
     underlying kernel descriptor, if it's still actually open --
     fd_null() itself never closes anything, it only marks the slot
     unusable -- has to be closed for real before an execve() can
     inherit it. This can't be routed through the ordinary FD_CLOSE
     case below: that case's job is closing whatever *other* fd
     struct currently occupies this slot to make room for (d), which
     is meaningless here since (d) itself is the occupant -- doing so
     would just recurse into fdtable_close() calling back into this
     same resolve. Respect FDTABLE_NOCLOSE like fdtable_close() does,
     and only actually close() when an effective fd is being forced
     (FDTABLE_LAZY calls defer, same as everything else here). */
  if(d->mode & FD_NULL) {
    if((flags & FDTABLE_FD) && !(flags & FDTABLE_NOCLOSE))
      close(d->n);

    return FDTABLE_DONE;
  }

  /* do not open/close if we don't need an effective d */
  if((flags & FDTABLE_FD) == FDTABLE_LAZY) {
    if(d->mode & (FD_OPEN | FD_CLOSE | FD_STRALLOC))
      return state;
  }

  if((flags & FDTABLE_FD) && d != fdtable[d->n]) {
    d->mode = (d->mode & FD_FREE) | FD_CLOSE;
  }

  /*    state = fdtable_close(d->n, flags);*/

  /* do some actions to resolve the effective file descriptor */
  switch(d->mode & (FD_OPEN | FD_CLOSE | FD_STRALLOC)) {
    /* we're forced to close */
    case FD_CLOSE: {
      state = fdtable_close(d->n, flags);
      break;
    }

      /* if the d is still to be opened then try that */
    case FD_OPEN: {
      state = fdtable_open(d, flags);
      break;
    }

      /* drop here-docs to temp files */
    case FD_STRALLOC: {
      if(FD_ISRD(d))
        state = fdtable_here(d, flags);
      break;
    }
  }

  /* if we're not done yet we have to force the effective d number */
  if(state != FDTABLE_DONE) {
    if(fd_ok(state))
      flags |= FDTABLE_CLOSE;

    /* try to move/duplicate the old effective file descriptor,
       maybe this returns a valid d number which we'll have to
       close */
    state = fdtable_dup(d, flags);

    if(fd_ok(state)) {
      /* if this is gonna change the expected d we do a lazy
         check before */
      if(fd_expected < state) {
        if(fdtable_lazy(-1, flags) == FDTABLE_ERROR)
          return FDTABLE_ERROR;
      }

      close(state);
      fdtable_untrack(state);
      state = FDTABLE_DONE;
    }
  }

#if defined(DEBUG_OUTPUT) && defined(DEBUG_FDTABLE) && !defined(SHFORMAT) && !defined(SHPARSE2AST)
  debug_open();
  buffer_puts(debug_output, COLOR_YELLOW "fdtable_resolve" COLOR_NONE "(");
  fd_dump(d, debug_output);
  buffer_puts(debug_output, ", ");
  debug_flags(flags, (const char* const[]){"LAZY", "MOVE", "FORCE", "NOCLOSE", "CLOSE"});
  buffer_puts(debug_output, ") = ");

  buffer_puts(debug_output, ((const char* const[]){"0", "DONE", "ERROR", "PENDING"})[-state]);
  debug_nl_fl();
#endif

  return state;
}

int
fdtable_resolve(struct fd* d, int flags) {
  int state;

  if(fdtable_resolve_depth >= FDTABLE_SIZE) {
    sh_error("fdtable: redirection cycle detected");
    return FDTABLE_ERROR;
  }

  fdtable_resolve_depth++;
  state = fdtable_resolve_1(d, flags);
  fdtable_resolve_depth--;

  return state;
}
