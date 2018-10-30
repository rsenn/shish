#include "sh.h"
#include "fdtable.h"
#include "fd.h"

/* initialize an (fd) struct by setting the defaults to all members
 * 
 * (except for the links which are initialized on fdtable_link()) 
 * ----------------------------------------------------------------------- */
void fd_init(struct fd *fd, int n, int flags) {
  /* (re-)initialize things */
  fd->mode = flags;
  fd->name = NULL;
  fd->n = n;
  fd->e = -1;
  fd->dup = NULL;
  fd->dev = 0;
/*  fdrefc = 0;*/
  
  fd->r = &fd->rb;
  fd->w = &fd->wb;
  
  buffer_init(fd->r, read, fd->e, NULL, 0);
  buffer_init(fd->w, write, fd->e, NULL, 0);
}

  
