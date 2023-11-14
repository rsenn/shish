#include "../fd.h"
#include "../job.h"
#include "../sh.h"
#include "../../lib/sig.h"
#include "../../lib/windoze.h"
#include <assert.h>

#if !WINDOWS_NATIVE
#include <unistd.h>
#endif

int job_pgrp;

/* forks off a job
 * ----------------------------------------------------------------------- */
int
job_fork(struct job* j, union node* node, int bgnd) {
  pid_t pid, pgrp;

assert(j);

#if !WINDOWS_NATIVE
  sig_block(SIGCHLD);
#endif

  /* fork the process */
  if((pid = fork()) == -1) {
    sh_error("fork failed");
    return -1;
  }

  /* in the child, set the process group and return */
  if(pid == 0) {
    sh_forked();

    pgrp = j && j->nproc ? j->procs[0].pid : sh_pid;

#if !WINDOWS_NATIVE
    setpgid(sh_pid, pgrp);

    if(fd_ok(job_terminal))
      /* and then give the child terminal access */
      if(!bgnd)
        tcsetpgrp(job_terminal, pgrp);
#endif
    return pid;
  }

  pgrp = pid;

  /* in the parent update the process list of the j */
  if(j) {
    struct proc* p = &j->procs[0 /*j->nproc*/];

    p->pid = pid;
    p->status = -1;

    if(j->nproc == 0)
      j->pgrp = pgrp;
    else
      pgrp = j->procs[0].pid;

    j->nproc++;
  }

  if(pgrp != job_pgrp && !bgnd) {
#if !WINDOWS_NATIVE
    if(fd_ok(job_terminal))
      tcsetpgrp(job_terminal, pid);
#endif
    job_pgrp = pid;
  }

#if !WINDOWS_NATIVE
  setpgid(pid, pgrp);
#endif
  return pid;
}
