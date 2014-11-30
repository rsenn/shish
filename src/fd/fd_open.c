#ifndef WIN32
#include <unistd.h>
#endif
#include "fd.h"
#include "sh.h"

/* prepare the fd for opening a file
 * 
 * it will be opened later when necessary by fd_open()
 * ----------------------------------------------------------------------- */
void fd_open(struct fd *fd, const char *fname, long mode)
{
  fd->mode |= mode|FD_FILE|FD_OPEN;
  fd->name = fname;

  /* set POSIX read/write mode */
  switch(fd->mode & FD_READWRITE)
  {
    case FD_READWRITE: fd->fl = O_RDWR; break;
    case FD_WRITE:     fd->fl = O_WRONLY; break;
    default:           fd->fl = O_RDONLY; break;
  }

  /* if we're opening a file for writing there are further options */
  if(FD_ISWR(fd))
  {
    /* append or truncate, never overwrite! */
    fd->fl |= ((mode & (FD_APPEND|FD_READ)) ? O_APPEND : O_TRUNC);
    
    /* exclude or create */
    fd->fl |= ((mode & (FD_EXCL)) ? O_EXCL : O_CREAT);
  }
  
#ifdef O_LARGEFILE
  fd->fl |= O_LARGEFILE;
#endif /* O_LARGEFILE */
}

