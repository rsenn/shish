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
pid_t job_bgpid;

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
    /* only fragment this job's members off into their own process
       group -- and hand them the terminal -- when job control is
       actually active (interactive, "set -m"). Without that gate,
       every pipeline member (and every "cmd &"/backgrounded compound
       command) ended up in a process group of its own regardless of
       mode, real bash never does that for a non-interactive script
       (confirmed directly: "sleep 3 | cat &" under non-interactive
       bash leaves both members in bash's own pgid, not a new one) --
       and since nothing then ever reassigns the *terminal's* actual
       foreground process group to match (job_terminal is only ever
       set up for a genuinely interactive session), a job's members
       simply never receive a terminal-generated SIGINT/SIGQUIT at
       all: only shish's own (still-foreground) process group does.
       Confirmed via a real repro: "sleep 5 | cat" run under a
       non-interactive shish, then Ctrl-C at the controlling terminal
       -- shish itself dies, but "sleep 5" (already setpgid()'d into
       its own group here) is left running, orphaned, completely
       unaffected. Leaving this job's members in shish's own process
       group instead means a single terminal SIGINT reaches shish and
       every currently-running job member simultaneously, same as
       bash. */
    if(sh->opts.monitor) {
      setpgid(sh_pid, pgrp);

      if(fd_ok(job_terminal))
        /* and then give the child terminal access */
        if(!bgnd)
          tcsetpgrp(job_terminal, pgrp);
    }

    /* the blocked mask survives exec(), so a program this child later
       execs (or a builtin/subshell it runs in-process) would otherwise
       inherit SIGCHLD blocked forever */
    sig_unblock(SIGCHLD);
#endif
    return pid;
  }

  pgrp = pid;

  /* "$!" -- for a backgrounded pipeline of several commands, this
     runs once per member in order, so the last one processed (the
     pipeline's last command) wins, matching bash */
  if(bgnd)
    job_bgpid = pid;

  /* in the parent update the process list of the j */
  if(j) {
    struct proc* p = &j->procs[index];

    p->pid = pid;
    p->status = -1;

    /* j->pgrp only names a *real* process group when job control put
       this job's members into one of their own (see the child branch
       above) -- left at 0 (job_new()'s default) otherwise, so
       job_wait() knows to fall back to waiting for any child instead
       of a group that doesn't exist. */
    if(sh->opts.monitor) {
      if(index == 0)
        j->pgrp = pgrp;
      else
        pgrp = j->procs[0].pid;
    }
  }

  if(sh->opts.monitor && pgrp != job_pgrp && !bgnd) {
#if !WINDOWS_NATIVE
    if(fd_ok(job_terminal))
      tcsetpgrp(job_terminal, pid);
#endif
    job_pgrp = pid;
  }

#if !WINDOWS_NATIVE
  if(sh->opts.monitor)
    setpgid(pid, pgrp);

  sig_unblock(SIGCHLD);
#endif
  return pid;
}
