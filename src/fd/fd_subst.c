#include <stralloc.h>
#include "fd.h"

/* prepare fd for command substitution stuff
 * ----------------------------------------------------------------------- */
void fd_subst(struct fd *fd, stralloc *sa)
{
  fd->name = "<subst>";
  fd->mode = FD_SUBST;
  
  buffer_init(fd->w, (ssize_t (*)())stralloc_write, (long)sa, NULL, 0);  
}

