#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "../../lib/windoze.h"
#if !WINDOWS_NATIVE
#include <termios.h>
#include <unistd.h>
#endif
#include "../fd.h"
#include "../job.h"
#include "../sh.h"
#include "../../lib/wait.h"

/* waits for a job to terminate
 * ----------------------------------------------------------------------- */
int
job_wait(struct job* job, int pid, int* status) {
  int ret = 0;

  if(job) {
    unsigned int n = job->nproc;

    while(n > 0) {
      unsigned int i;
      int st; /* status */

      if((ret = wait_pid(job->pgrp ? -job->pgrp : job->procs[0].pid, &st)) <= 0)
        break;

      for(i = 0; i < job->nproc; i++)
        if(job->procs[i].pid == ret)
          n--;

      if(ret == job->pgrp)
        *status = st;

      job_status(ret, st);
    }
  } else {
    /* wait for the last process in the group to terminate */
    ret = wait_pid(pid, status);
  }

  if(job_pgrp != sh_pid) {
    if(fd_ok(job_terminal)) {
#if !WINDOWS_NATIVE
      setpgid(sh_pid, sh_pid);
      tcsetpgrp(job_terminal, sh_pid);
#endif
    }
  }

  return ret;
}
