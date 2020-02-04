#include "fd.h"
#include "stralloc.h"

/* prepare fd for command substitution stuff
 * ----------------------------------------------------------------------- */
void
fd_subst(struct fd* fd, stralloc* sa) {
  fd->name = "<subst>";
  fd->mode = D_SUBST;

  buffer_init(fd->w, (ssize_t(*)())stralloc_write, sa, NULL, 0);
}
