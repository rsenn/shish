#include "fd.h"
#include "fdtable.h"

/* link fd to the table at the current position
 * ----------------------------------------------------------------------- */
void
fdtable_link(struct fd* fd) {
  fd->pos = fdtable_pos;
  fd->parent = *fd->pos;
  *fd->pos = fd;

  if(fd->parent)
    fd->parent->pos = &fd->parent;

  if(fdtable_lo > fd->n)
    fdtable_lo = fd->n;

  if(fdtable_hi <= fd->n)
    fdtable_hi = fd->n + 1;
}
