#include "fd.h"
#include "fmt.h"
#include "sh.h"
#include <errno.h>
#include <string.h>

/* make an (fd) entry a duplicate of another
 * ----------------------------------------------------------------------- */
int
fd_dup(struct fd* fd, int n) {
  struct fd* dupe;

  /* search the (fd) to be duplicated and
     output error message and return if not found */
  if((dupe = fdtable[n]) == NULL)
    return fd_error(n, strerror(EBADF));

  while(dupe->dup) dupe = dupe->dup;

  if(dupe == fd)
    return fd_error(n, "we cannot duplicate ourselves");

  /* redirect buffer pointers */
  if(D_ISRD(fd)) {
    if(!D_ISRD(dupe))
      return fd_error(n, "not opened for reading");
  }

  if(D_ISWR(fd)) {
    if(!D_ISWR(dupe))
      return fd_error(n, "not opened for writing");
  }

  fd->r = dupe->r;
  fd->w = dupe->w;

  /* copy the name and set up the mode of the duplicate */
  fd->name = dupe->name;
  fd->dup = dupe;
  fd->e = dupe->e;
  fd->mode |= (dupe->mode & D_TYPE) | D_DUP;
  fd->dev = dupe->dev;

  return 0;
}
