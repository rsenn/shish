#include "fd.h"
#include "job.h"
#include <fcntl.h>
#include <string.h>
#include "../../lib/windoze.h"
#if !WINDOWS_NATIVE && !defined(__MINGW64__)
#include <termios.h>
#include <unistd.h>
#else
#include <io.h>
#endif

int job_terminal = -1;

/* initializes job control
 * ----------------------------------------------------------------------- */
void
job_init(void) {
  struct fd* fd;

  /* find a filedescriptor which is a terminal */
  if((fd = fdtable[STDERR_FILENO]) && (fd_err->mode & D_TERM)) {
#include "../../lib/windoze.h"
#if !WINDOWS_NATIVE && !defined(__MINGW64__)
    job_terminal = fcntl(fd->e, F_DUPFD, 0x80);
#else
    job_terminal = dup(fd->e);
#endif

#ifdef FD_CLOEXEC
    fcntl(job_terminal, F_SETFD, FD_CLOEXEC);
#endif
#if !WINDOWS_NATIVE
    job_pgrp = tcgetpgrp(job_terminal);
#endif
  }
}
