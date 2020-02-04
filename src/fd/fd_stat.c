#include "fd.h"

/* stat the (fd) and set appropriate flags
 * ----------------------------------------------------------------------- */
int
fd_stat(struct fd* fd) {
  struct stat st;

  if(fd->mode & D_TYPE)
    return 0;

  if(fstat(fd->n, &st) == -1)
    return 1;

  fd->dev = st.st_rdev;

  switch(st.st_mode & S_IFMT) {
    case S_IFREG: fd->mode |= D_FILE; break;
    case S_IFDIR: fd->mode |= D_DIR; break;
    case S_IFCHR: fd->mode |= D_CHAR; break;
    case S_IFBLK: fd->mode |= D_BLOCK; break;
    case S_IFIFO: fd->mode |= D_PIPE; break;
#ifdef S_IFLNK
    case S_IFLNK: fd->mode |= D_LINK; break;
#endif
#ifdef S_IFSOCK
    case S_IFSOCK: fd->mode |= D_SOCKET; break;
#endif
  }

  return 0;
}
