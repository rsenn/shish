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
  int index = -1;

  assert(j);

  /* find the next unclaimed proc slot -- job_new() zeroes every
     procs[i].pid to 0 up front. j->nproc is the job's fixed total
     member count, set once by job_new() before the first fork ever
     happens (e.g. to a pipeline's member count) -- it's not a
     running "how many forked so far" counter, so it can't be used to
     pick an index or to detect "am I the first member" (both the
     child and parent branches below used to do exactly that, and
     both were wrong: nproc is never 0 by the time job_fork() runs at
     all). Confirmed with an ASan build: any 2+-process job wrote past
     the end of its own procs[] array. */
  if(j) {
    unsigned int i;

    for(i = 0; i < j->nproc; i++)
      if(j->procs[i].pid == 0) {
        index = (int)i;
        break;
      }

    assert(index >= 0);
  }

#if !WINDOWS_NATIVE
  sig_block(SIGCHLD);
#endif

  /* fork the process */
  if((pid = fork()) == -1) {
#if !WINDOWS_NATIVE
    sig_unblock(SIGCHLD);
#endif
    sh_error_errno("fork failed");
    return -1;
  }

  /* in the child, set the process group and return */
  if(pid == 0) {
    sh_forked();

    pgrp = index > 0 ? j->procs[0].pid : sh_pid;

#if !WINDOWS_NATIVE
    setpgid(sh_pid, pgrp);

    if(fd_ok(job_terminal))
      /* and then give the child terminal access */
      if(!bgnd)
        tcsetpgrp(job_terminal, pgrp);

    /* the blocked mask survives exec(), so a program this child later
       execs (or a builtin/subshell it runs in-process) would otherwise
       inherit SIGCHLD blocked forever */
    sig_unblock(SIGCHLD);
#endif
    return pid;
  }

  pgrp = pid;

  /* in the parent update the process list of the j */
  if(j) {
    struct proc* p = &j->procs[index];

    p->pid = pid;
    p->status = -1;

    if(index == 0)
      j->pgrp = pgrp;
    else
      pgrp = j->procs[0].pid;
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
  sig_unblock(SIGCHLD);
#endif
  return pid;
}
