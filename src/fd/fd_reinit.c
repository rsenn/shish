<<<<<<< HEAD
#ifdef WIN32
=======
#include "shell.h"
#ifdef _WIN32
>>>>>>> 6c7455723b47a4989fb5bb621be8f200a306f361
#include <io.h>
#else
#include <unistd.h>
#endif
<<<<<<< HEAD
#include "shell.h"
=======
>>>>>>> 6c7455723b47a4989fb5bb621be8f200a306f361
#include "fd.h"

/* reinitialize an (fd) struct
 * 
 * (except for the links which are initialized on fdtable_link()) 
 * ----------------------------------------------------------------------- */
struct fd *fd_reinit(struct fd *fd, int flags)
{
  /* unset the name, and if it was allocated: free it */
  if(fd->name)
  {
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

  
