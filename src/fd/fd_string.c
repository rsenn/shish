#include "fd.h"

/* prepare fd for reading from a string
 * ----------------------------------------------------------------------- */
void fd_string(struct fd *fd, const char *s, unsigned long len)
{
  fd->mode = FD_STRING;
  fd->name = "<string>";

  buffer_fromstr(fd->r, (char *)s, len);
}

