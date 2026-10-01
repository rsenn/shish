#include "../trace.h"
#include "../fd.h"
#include "../../lib/alloc.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif
#include "../../lib/byte.h"

/* restore the bookkeeping fd_state_save() snapshotted, and undo what the
 * scope did to fds an outer level owns (see fd_scope_note()).
 *
 *   struct fd  e: 1 -> 4    "exec >/dev/null" relocated the real stdout
 *   undone by  dup2(4, 1); close(4); fd->e = 1
 *
 * Descriptors the scope itself opened were closed by fdstack_pop().
 * ----------------------------------------------------------------------- */
void
fd_state_restore(struct fd_state* st) {
  struct fd_note *n, *next;

  TRACE(TRACE_FD, "state.restore", trace_int("expected", st->expected), trace_int("lo", st->lo), trace_int("hi", st->hi));

  /* fds an outer level owns, changed inside the scope: back to their
     original descriptor ("exec >/dev/null" moved the real stdout away) */
  for(n = st->notes; n; n = next) {
    struct fd* d = n->fd;

    next = n->next;

    if(d->e != n->e && !(d->mode & FD_DUP) && fd_ok(n->e) && fd_ok(d->e)) {
      dup2(d->e, n->e);
      close(d->e);
    }

    d->e = n->e;
    d->rb.fd = n->rfd;
    d->wb.fd = n->wfd;
    d->r = n->r;
    d->w = n->w;
    alloc_free(n);
  }

  st->notes = NULL;
  fd_scope = st->prev;

  fd_expected = st->expected;
  fd_top = st->top;
  fd_lo = st->lo;
  fd_hi = st->hi;
  byte_copy(fd_list, sizeof(fd_list), st->list);
}
