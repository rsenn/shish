#include "../trace.h"
#include "../fd.h"
#include "../../lib/stralloc.h"

/* prepare fd for command substitution stuff
 * ----------------------------------------------------------------------- */
void
fd_subst(struct fd* d, stralloc* sa) {
  TRACE(TRACE_FD, "subst", trace_int("n", d->n));

  d->name = "<subst>";
  d->mode = (d->mode & FD_FREE) | FD_SUBST;

  buffer_init(d->w, &stralloc_write, -1, NULL, 0);
  d->w->cookie = sa;
}
