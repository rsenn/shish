#include "../fd.h"
#include "../fdtable.h"
#include "../../lib/fmt.h"
#include "../sh.h"
#include <errno.h>
#include <string.h>

/* make an (fd) entry a duplicate of another
 * ----------------------------------------------------------------------- */
int
fd_dup(struct fd* d, int n) {
  struct fd* dupe;

  /* search the (d) to be duplicated and
     output error message and return if not found */
  if((dupe = fdtable[n]) == NULL)
    return fd_error(n, strerror(EBADF));

  while(dupe->dup)
    dupe = dupe->dup;

  if(dupe == d)
    return fd_error(n, "we cannot duplicate ourselves");

  /* redirect buffer pointers */
  if(FD_ISRD(d)) {
    if(!FD_ISRD(dupe))
      return fd_error(n, "not opened for reading");
  }

  if(FD_ISWR(d)) {
    if(!FD_ISWR(dupe))
      return fd_error(n, "not opened for writing");
  }

  d->r = dupe->r;
  d->w = dupe->w;

  /* copy the name and set up the mode of the duplicate */
  d->name = dupe->name;
  d->dup = dupe;
  d->e = dupe->e;
  d->mode |= (dupe->mode & FD_TYPE) | FD_DUP;
  d->dev = dupe->dev;

  return 0;
}
