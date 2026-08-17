#include "../fd.h"
#include "../fdtable.h"
#include "../sh.h"

/* initialize an (fd) struct by setting the defaults to all members
 *
 * (except for the links which are initialized on fdtable_link())
 * ----------------------------------------------------------------------- */
void
fd_init(struct fd* d, int n, int mode) {
  /* (re-)initialize things */
  d->mode = mode;
  d->name = NULL;
  d->n = n;
  d->e = -1;
  d->dup = NULL;
  d->dev = 0;
  /*  fdrefc = 0;*/

  d->r = &d->rb;
  d->w = &d->wb;

  buffer_init(d->r, &buffer_op_read, d->e, NULL, 0);
  buffer_init(d->w, &buffer_op_write, d->e, NULL, 0);
}
