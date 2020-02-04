#include "fd.h"
#include "stralloc.h"

/* prepare fd for command substitution stuff
 * ----------------------------------------------------------------------- */
void
fd_subst(struct fd* fd, stralloc* sa) {
  fd->name = "<subst>";
  fd->mode = D_SUBST;

  buffer_init(fd->w, (buffer_op_proto*)&stralloc_write, -1, NULL, 0);
  fd->w->cookie = &sa;
}
