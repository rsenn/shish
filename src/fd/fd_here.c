#include "fd.h"

/* prepare (fd) for reading from a stralloc
 * 
 * control of the allocated sa->s member is passed to the buffer
 * (means it will be free'd wenn the (fd) is closed)
 * ----------------------------------------------------------------------- */
void fd_here(struct fd *fd, stralloc *sa) {
  fd->name = "<here>";
  fd->mode = FD_HERE;
  
  buffer_fromsa(&fd->rb, sa);
  fd->rb.deinit = buffer_free;
  fd->e = -1;
}

