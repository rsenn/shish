#include "../fd.h"
#include "../fdstack.h"
#include "../fdtable.h"
#include "../../lib/windoze.h"
#include <errno.h>
#include <fcntl.h>
#if WINDOWS_NATIVE
#include <io.h>
#define dup _dup
#else
#include <unistd.h>
#endif

/* request a gap below fd_expected, a gap can be created if the fd
 * is owned by an fd not on top. a gap can also be forced through
 * fd_resolve.
 *
 * fd is assumed to be smaller than 'fd_expected'
 *
 * return 1 if we made a gap
 * ----------------------------------------------------------------------- */
int
fdtable_gap(int e, int flags) {
  struct fd* gap;

  /* there is already a gap? */
  if((gap = fd_list[e]) == NULL)
    return FDTABLE_DONE;

  /* gap is explicitly due to be closed (e.g. a resolved temp-file
     redirect, FD_CLOSE) -- nothing will ever need it again, so it's
     genuinely safe to destroy for real. Neuter its real fd first so
     fd_close() -> buffer_close() has nothing left to close() -- the
     caller may still need this exact kernel fd number a moment
     longer (e.g. a pipe endpoint about to be dup2()'d in a forked
     child), and will re-establish it via dup2()/open() landing back
     on "e". */
  if(gap->mode & FD_CLOSE) {
    fd_setfd(gap, -1);
    fd_pop(gap);

    return (flags & FDTABLE_NOCLOSE) ? e : FDTABLE_DONE;
  }

  /* gap is merely *shadowed*: some other, newer struct is already the
     active virtual owner of gap's own slot (gap != fdtable[gap->n]),
     but gap itself is still linked into an outer fdstack level,
     waiting to become active again once that shadow pops -- e.g. the
     process's own top-level fd 0/1/2 struct, shadowed for the
     duration of one command's redirection. Destroying gap here (the
     original behavior, unconditionally) left nothing for that pop to
     re-expose: fdtable[gap->n] would go from "shadow" straight to
     empty instead of back to gap, permanently losing shish's own
     connection to whatever real resource gap represented (confirmed
     via a heredoc-fed external command: "cat >file <<EOF" borrows
     real fd 0 for its heredoc's temp file, evicting the shell's own
     inherited stdin struct this way -- once "cat" finishes and its
     redirection scope pops, virtual fd 0 should revert to that
     inherited stdin, but the struct destroy above had already freed
     it, so every pipe()/open() afterward was free to reuse real fd 0
     for something unrelated, silently colliding with whatever next
     assumed it still meant "the process's real stdin").
     Relocating gap's real backing to a fresh fd (a genuine dup(),
     independent of "e") frees "e" up exactly the same as destroying
     gap did, but keeps gap's struct/fdstack identity intact so the
     eventual pop still finds it. */
  if((flags & FDTABLE_FORCE) && gap != fdtable[gap->n]) {
    int newfd = dup(gap->e);

    if(newfd == -1)
      return FDTABLE_ERROR;

    fd_setfd(gap, newfd);

    return (flags & FDTABLE_NOCLOSE) ? e : FDTABLE_DONE;
  }

  if(flags & FDTABLE_FORCE) {
    /* resolve the gap fd, but do not force
       position to prevent from infinite recursion */
    flags &= ~FDTABLE_FORCE;
    flags |= FDTABLE_MOVE;

    return fdtable_resolve(gap, flags);
  }

  return FDTABLE_PENDING;
}
