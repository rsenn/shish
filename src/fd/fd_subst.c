#include "../fd.h"
#include "../../lib/stralloc.h"

/* prepare fd for command substitution stuff
 * ----------------------------------------------------------------------- */
void
fd_subst(struct fd* d, stralloc* sa) {
  d->name = "<subst>";
  d->mode = FD_SUBST;

  buffer_init(d->w, (buffer_op_proto*)(void*)&stralloc_write, -1, NULL, 0);
  d->w->cookie = sa;
}
