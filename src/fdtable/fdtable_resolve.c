#include "../fd.h"
#include "../fdtable.h"
#include "../debug.h"
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
fdtable_resolve(struct fd* d, int flags) {
  int state = FDTABLE_PENDING;

  /* already resolved */
  if(d->e == d->n)
    return FDTABLE_DONE;

  /* do not open/close if we don't need an effective d */
  if((flags & FDTABLE_FD) == FDTABLE_LAZY) {
    if(d->mode & (FD_OPEN | FD_CLOSE | FD_STRALLOC))
      return state;
  }

  if((flags & FDTABLE_FD) && d != fdtable[d->n]) {
    d->mode = FD_CLOSE;
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
