#include <stdlib.h>
<<<<<<< HEAD
#ifndef WIN32
=======
#ifdef _WIN32
#include <io.h>
#else
>>>>>>> 6c7455723b47a4989fb5bb621be8f200a306f361
#include <unistd.h>
#endif
#include "shell.h"
#include "open.h"
#include "str.h"
#include "config.h"
#include "redir.h"
#include "fd.h"
#include "fdtable.h"

#include "debug.h"

#define FD_TEMPPFX "/tmp/"PACKAGE_NAME"-"
#define FD_TEMPLEN (sizeof(FD_TEMPPFX) + 6)

char fd_tempname[FD_TEMPLEN] = FD_TEMPPFX;

/* opens a temporary file and adds it as the specified virtual fd
 * ----------------------------------------------------------------------- */
int fd_tempfile(struct fd *fd)
{
  int e;
  
  /* reset the template to what mkstemp expects */
  str_copy(&fd_tempname[sizeof(fd_tempname)-7], "XXXXXX");

  /* try to create a tempfile and exit on failure 
     otherwise track the newly created file descriptor */
  if((e = open_temp(fd_tempname)) == -1)
    return e;
  
  fdtable_track(e, FDTABLE_LAZY);
  
  /* tempfiles are initially in write mode only */
  fd->mode = FD_WRITE;

  fd_setfd(fd, e);

  /* unlink the file, the fd should now be the only reference */
  unlink(fd_tempname);

  /* make a copy of the name because the 
     next call will change fd_tempname */
  fd->name = shell_strdup(fd_tempname);
  fd->mode |= FD_FREENAME;
  
  return e;
}

