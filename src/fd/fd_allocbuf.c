#include <shell.h>
#include "fd.h"

/* allocate buffer space for the (fd).
 * this should only be called when the (fd) really lacks buffer space!
 * ----------------------------------------------------------------------- */
void fd_allocbuf(struct fd *fd, unsigned long n)
{
  char *p = shell_alloc(n);

  fd_setbuf(fd, p, n);
}
