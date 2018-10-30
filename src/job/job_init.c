#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include "job.h"
#include "fd.h"

int job_terminal = -1;

/* initializes job control
 * ----------------------------------------------------------------------- */
void job_init(void) {
  struct fd *fd;
  
  /* find a filedescriptor which is a terminal */
  if((fd = fdtable[STDERR_FILENO]) && (fd_err->mode & FD_TERM)) {
    job_terminal = fcntl(fd->e, F_DUPFD, 0x80);

    fcntl(job_terminal, F_SETFD, FD_CLOEXEC);
    job_pgrp = tcgetpgrp(job_terminal);
  }
}

