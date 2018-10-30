#include "shell.h"
#include <unistd.h>
#include "fd.h"

/* reinitialize an (fd) struct
 * 
 * (except for the links which are initialized on fdtable_link()) 
 * ----------------------------------------------------------------------- */
struct fd *fd_reinit(struct fd *fd, int flags) {
  /* unset the name, and if it was allocated: free it */
  if(fd->name) {
    if(fd->mode & FD_FREENAME)
      shell_free((char *)fd->name);

    fd->name = NULL;
  }
     
  fd_close(fd);
  
  /* re-initialize things */
  fd->mode &= FD_FREE;
  fd->mode |= flags;

  fd->dup = NULL;
  fd->dev = 0;
  fd->e = -1;
  
  fd->r = &fd->rb;
  fd->w = &fd->wb;
  
  buffer_default(&fd->rb, read);
  buffer_default(&fd->wb, write);
  
  return fd;
}

  
