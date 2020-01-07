#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <termios.h>
#include <unistd.h>
#if defined(HAVE_SYS_WAIT_H) && !defined(__MINGW32__)
#include <sys/wait.h>
#endif
#include "fd.h"
#include "job.h"
#include "sh.h"

/* waits for a job to terminate
 * ----------------------------------------------------------------------- */
int
job_wait(struct job* job, int pid, int* status, int options) {
  int ret = 0;
  int st; /* status */

  if(job) {
    int n = job->nproc;

    while(n > 0) {
      int i;

      ret = waitpid(-job->pgrp, &st, options);

      if(ret <= 0)
        break;

      for(i = 0; i < job->nproc; i++) {
        if(job->procs[i].pid == ret)
          n--;
      }

      if(ret == job->pgrp)
        *status = st;

      job_status(ret, st);
    }
  } else {
    /* wait for the last process in the group to terminate */
    ret = waitpid(pid, status, options);
  }

  if(job_pgrp != sh_pid) {
    if(fd_ok(job_terminal)) {
      setpgid(sh_pid, sh_pid);
      tcsetpgrp(job_terminal, sh_pid);
    }
  }

  return ret;
}
