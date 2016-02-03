<<<<<<< HEAD
#ifndef WIN32
=======
#ifdef _WIN32
#include <io.h>
#else
>>>>>>> 6c7455723b47a4989fb5bb621be8f200a306f361
#include <unistd.h>
#endif
#include <termios.h>
#endif
#include <sys/wait.h>
#include "job.h"
#include "fd.h"
#include "sh.h"

/* waits for a job to terminate
 * ----------------------------------------------------------------------- */
int job_wait(struct job *job, int pid, int *status, int options)
{
  int ret = 0;
  int st; /* status */

  if(job)
  {
    int n = job->nproc;

    while(n > 0)
    {
      int i;

      ret = waitpid(-job->pgrp, &st, options);

      if(ret <= 0)
        break;

      for(i = 0; i < job->nproc; i++)
      {
        if(job->procs[i].pid == ret)
          n--;
      }

      if(ret == job->pgrp)
        *status = st;
      
      job_status(ret, st);
    }
  }
  else
  {
    /* wait for the last process in the group to terminate */
    ret = waitpid(pid, status, options);
  }

  if(job_pgrp != sh_pid)
  {
    if(fd_ok(job_terminal))
    {
      setpgid(sh_pid, sh_pid);
      tcsetpgrp(job_terminal, sh_pid);
    }
  }

  return ret;
}


