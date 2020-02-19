#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../fd.h"
#include "../../lib/windoze.h"

#if !WINDOWS_NATIVE
#include <fcntl.h>
#endif

/* check for the presence of an effective file descriptor
 *
 * returns FD_* flags or 0 if the check failed
 * ----------------------------------------------------------------------- */
int
fdtable_check(int e) {
  int pflags;
  int iflags = 0;

  /* try to get file descriptor flags */
#if !WINDOWS_NATIVE
  if((pflags = fcntl(e, F_GETFL)) == -1)
    return 0;
#else
  pflags = O_RDWR;
#endif

  /* map posix file access modes to (fd) flags */
  if(pflags & (O_RDWR | O_WRONLY))
    iflags |= FD_WRITE;
  if((pflags & O_WRONLY) == 0)
    iflags |= FD_READ;
  if(pflags & O_APPEND)
    iflags |= FD_APPEND;
  if(pflags & O_TRUNC)
    iflags |= FD_TRUNC;
  if(pflags & O_EXCL)
    iflags |= FD_EXCL;

  return iflags;
}
