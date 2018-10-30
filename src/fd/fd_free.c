#include "shell.h"
#include "fd.h"

/* free associated (fd) ressources and possibly the fd struct itself 
 * ----------------------------------------------------------------------- */
void fd_free(struct fd *fd) {
  /* unset the name, and if it was allocated: free it */
  if(fd->name) {
    if(fd->mode & FD_FREENAME)
      shell_free((char *)fd->name);
    
    fd->name = NULL;
  }
  
  /* if the (fd) struct itself was allocated we have to free it */
  if(fd->mode & FD_FREE)
    shell_free(fd);
}
