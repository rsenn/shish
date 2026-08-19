#include "../fd.h"
#include "../../lib/byte.h"

/* restore fd_expected/fd_top/fd_lo/fd_hi/fd_list[] from a snapshot
 * taken by fd_state_save() -- see that function's comment.
 *
 * This undoes shish's own bookkeeping about which real kernel fd
 * numbers are free/owned; it does NOT undo any actual dup2()/close()
 * syscall a persistent ("exec") redirection inside the scope may have
 * issued against a real descriptor still live at this point (e.g. fd
 * 0/1/2) -- a real kernel fd, once changed, stays changed. Callers
 * that can run persistent redirections in a non-forking scope still
 * need those redirections to behave as scope-local for the change to
 * be fully correct; this only closes the bookkeeping half of the gap
 * (see TODO.md, Goal 4).
 * ----------------------------------------------------------------------- */
void
fd_state_restore(const struct fd_state* st) {
  fd_expected = st->expected;
  fd_top = st->top;
  fd_lo = st->lo;
  fd_hi = st->hi;
  byte_copy(fd_list, sizeof(fd_list), st->list);
}
