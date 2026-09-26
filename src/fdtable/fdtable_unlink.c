#include "../trace.h"
#include "../fd.h"

/* unlinks an fd from the stack
 * ----------------------------------------------------------------------- */
void
fdtable_unlink(struct fd* fd) {
  TRACE(TRACE_FDTABLE, "unlink", trace_int("n", fd->n));

  if(fd->parent)
    fd->parent->pos = fd->pos;

  *fd->pos = fd->parent;
}
