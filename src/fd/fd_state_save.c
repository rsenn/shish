#include "../trace.h"
#include "../fd.h"
#include "../fdstack.h"
#include "../../lib/alloc.h"
#include "../../lib/byte.h"

struct fd_state* fd_scope;

/* remember fd's e/buffer fds before a scope changes them, if an outer
 * level owns it; fd_state_restore() puts them back.
 *
 *   struct fd*  d  the fd about to change
 * ----------------------------------------------------------------------- */
void
fd_scope_note(struct fd* d) {
  struct fd_note* n;

  if(!fd_scope || !d->stack || d->stack->level >= fd_scope->level)
    return;

  for(n = fd_scope->notes; n; n = n->next)
    if(n->fd == d)
      return;

  n = alloc(sizeof(struct fd_note));
  n->fd = d;
  n->e = d->e;
  n->rfd = d->rb.fd;
  n->wfd = d->wb.fd;
  n->r = d->r;
  n->w = d->w;
  n->next = fd_scope->notes;
  fd_scope->notes = n;
}

/* open a non-forking fd scope: snapshot fd_expected/fd_top/fd_lo/fd_hi/
 * fd_list[] and start journaling changes to fds owned by outer levels.
 * Callers (eval_subshell, expand_command) push an fdstack level first, then
 * call fd_state_save(), and pair it with fd_state_restore() after the pop.
 * ----------------------------------------------------------------------- */
void
fd_state_save(struct fd_state* st) {
  TRACE(TRACE_FD, "state.save", trace_int("expected", fd_expected), trace_int("lo", fd_lo), trace_int("hi", fd_hi));

  st->level = fdstack->level;
  st->prev = fd_scope;
  st->notes = NULL;
  fd_scope = st;
  st->expected = fd_expected;
  st->top = fd_top;
  st->lo = fd_lo;
  st->hi = fd_hi;
  byte_copy(st->list, sizeof(st->list), fd_list);
}
