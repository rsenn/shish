#include "../trace.h"
#include "../fd.h"
#include "../../lib/byte.h"

/* snapshot fd_expected/fd_top/fd_lo/fd_hi/fd_list[] -- the real-kernel-
 * fd bookkeeping globals fdstack_push()/fdstack_pop() don't already
 * scope per level, unlike the struct fd entries themselves. A caller
 * that runs a redirection-capable scope in the current process (no
 * fork()) -- currently only eval_subshell.c -- pairs this with
 * fd_state_restore() around that scope, the same way it already pairs
 * vartab_push()/vartab_pop().
 * ----------------------------------------------------------------------- */
void
fd_state_save(struct fd_state* st) {
  TRACE(TRACE_FD, "state.save", trace_int("expected", fd_expected), trace_int("lo", fd_lo), trace_int("hi", fd_hi));

  st->expected = fd_expected;
  st->top = fd_top;
  st->lo = fd_lo;
  st->hi = fd_hi;
  byte_copy(st->list, sizeof(st->list), fd_list);
}
