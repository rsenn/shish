#include <unistd.h>
#include "job.h"
#include "sig.h"
#include "sh.h"
#include "fd.h"

int job_pgrp;

/* forks off a job
 * ----------------------------------------------------------------------- */
int job_fork(struct job *job, union node *node, int bgnd)
{
  pid_t pid;
  pid_t pgrp;

  sig_block();
  
  /* fork the process */
  if((pid = fork()) == -1)
  {
    sh_error("fork failed");
    return -1;
  }

  /* in the child, set the process group and return */
  if(pid == 0)
  {
    sh_forked();

    if(job && job->nproc)
      pgrp = job->procs[0].pid;
    else
      pgrp = sh_pid;

    setpgid(sh_pid, pgrp);

    if(fd_ok(job_terminal))
      /* and then give the child terminal access */
      if(!bgnd)
        tcsetpgrp(job_terminal, pgrp);
  
    return pid;
  }
    
  pgrp = pid;
  
  /* in the parent update the process list of the job */
  if(job)
  {
    struct proc *proc = &job->procs[job->nproc];
    proc->pid = pid;
    proc->status = -1;
    
    if(job->nproc == 0)
      job->pgrp = pgrp;
    else
      pgrp = job->procs[0].pid;
    
    job->nproc++;
  }
    
  if(pgrp != job_pgrp && !bgnd)
  {
    if(fd_ok(job_terminal))
      tcsetpgrp(job_terminal, pid);

    job_pgrp = pid;
  }
  
  return pid;
}

