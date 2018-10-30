#include "fd.h"

/* stat the (fd) and set appropriate flags
 * ----------------------------------------------------------------------- */
int fd_stat(struct fd *fd) {
  struct stat st;
  
  if(fd->mode & FD_TYPE)
    return 0;
  
  if(fstat(fd->n, &st) == -1)
    return 1;
  
  fd->dev = st.st_rdev;
  
  switch(st.st_mode & S_IFMT) {
    case S_IFREG: fd->mode |= FD_FILE; break;
    case S_IFDIR: fd->mode |= FD_DIR; break;
    case S_IFCHR: fd->mode |= FD_CHAR; break;
    case S_IFBLK: fd->mode |= FD_BLOCK; break;
    case S_IFIFO: fd->mode |= FD_PIPE; break;
#ifdef S_IFLNK
    case S_IFLNK: fd->mode |= FD_LINK; break;
#endif
#ifdef S_IFSOCK
    case S_IFSOCK: fd->mode |= FD_SOCKET; break;
#endif
  }
  
  return 0;
}
