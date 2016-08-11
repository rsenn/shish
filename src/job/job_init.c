#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#if !defined(_MSC_VER) && !defined(__MINGW32__)
#include <termios.h>
#else
#include <windows.h>
#endif
#include "job.h"
#include "fd.h"

int job_terminal = -1;

/* initializes job control
 * ----------------------------------------------------------------------- */
void job_init(void) {
  struct fd *fd;
  
  /* find a filedescriptor which is a terminal */
  if((fd = fdtable[STDERR_FILENO]) && (fd_err->mode & FD_TERM)) {
  
#if !defined(_MSC_VER) && !defined(__MINGW32__)
    job_terminal =   fcntl(fd->e, F_DUPFD, 0x80);
    fcntl(job_terminal, F_SETFD, FD_CLOEXEC);
    job_pgrp = tcgetpgrp(job_terminal);
#else
     HANDLE h;
     DuplicateHandle(GetCurrentProcess(), (HANDLE)(intptr_t)fd->e, GetCurrentProcess(), &h, 0, FALSE,  DUPLICATE_SAME_ACCESS );
     job_terminal = (intptr_t)h;
#endif
  }
}

