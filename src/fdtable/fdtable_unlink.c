#include "fd.h"

/* unlinks an fd from the stack
 * ----------------------------------------------------------------------- */
void
fdtable_unlink(struct fd* fd) {
  if(fd->parent)
    fd->parent->pos = fd->pos;

  *fd->pos = fd->parent;
}
