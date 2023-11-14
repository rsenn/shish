#include "../fd.h"
#include "../sh.h"
#include "../../lib/windoze.h"
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
fd_open(struct fd* d, const char* fname, long mode) {
  d->mode |= mode | FD_FILE | FD_OPEN;
  d->name = fname;

  /* set POSIX read/write mode */
  switch(d->mode & FD_READWRITE) {
    case FD_READWRITE: d->fl = O_RDWR; break;
    case FD_WRITE: d->fl = O_WRONLY; break;
    default: d->fl = O_RDONLY; break;
  }

  /* if we're opening a file for writing there are further options */
  if(FD_ISWR(d)) {
    /* append or truncate, never overwrite! */
    d->fl |= ((mode & (FD_APPEND | FD_READ)) ? O_APPEND : O_TRUNC);

    /* exclude or create */
    d->fl |= /*(mode & (FD_EXCL)) ? O_EXCL :*/ O_CREAT;
  }

#ifdef O_LARGEFILE
  d->fl |= O_LARGEFILE;
#endif /* O_LARGEFILE */
}
