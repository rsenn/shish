#include "fd.h"
#include "sh.h"
#include "windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* prepare the fd for opening a file
 *
 * it will be opened later when necessary by fd_open()
 * ----------------------------------------------------------------------- */
void
fd_open(struct fd* fd, const char* fname, long mode) {
  fd->mode |= mode | D_FILE | D_OPEN;
  fd->name = fname;

  /* set POSIX read/write mode */
  switch(fd->mode & D_READWRITE) {
    case D_READWRITE: fd->fl = O_RDWR; break;
    case D_WRITE: fd->fl = O_WRONLY; break;
    default: fd->fl = O_RDONLY; break;
  }

  /* if we're opening a file for writing there are further options */
  if(D_ISWR(fd)) {
    /* append or truncate, never overwrite! */
    fd->fl |= ((mode & (D_APPEND | D_READ)) ? O_APPEND : O_TRUNC);

    /* exclude or create */
    fd->fl |= ((mode & (D_EXCL)) ? O_EXCL : O_CREAT);
  }

#ifdef O_LARGEFILE
  fd->fl |= O_LARGEFILE;
#endif /* O_LARGEFILE */
}
