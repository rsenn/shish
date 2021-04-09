#include "../fd.h"
#include "../../lib/stralloc.h"

/* prepare fd for command substitution stuff
 * ----------------------------------------------------------------------- */
void
fd_subst(struct fd* fd, stralloc* sa) {
  fd->name = "<subst>";
  fd->mode = FD_SUBST;

  buffer_init(fd->w, (buffer_op_proto*)(void*)&stralloc_write, -1, NULL, 0);
  fd->w->cookie = sa;
}
