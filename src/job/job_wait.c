#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "../../lib/windoze.h"
#if !WINDOWS_NATIVE
#include <termios.h>
#include <unistd.h>
#endif
#include "../job.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../../lib/wait.h"
#include <signal.h>

/* waits for a job to terminate
 * ----------------------------------------------------------------------- */
int
job_wait(struct job* j, pid_t pid, int* status) {
  int ret = 0, s;
  size_t n, i;

  if(j) {
    n = j->nproc;

    while(n > 0) {
      if((ret = wait_pid(j->pgrp ? -j->pgrp : j->procs[0].pid, &s)) <= 0)
        break;

      if(job_signal(ret, s))
        n--;

      /*for(i = 0; i < j->nproc; i++) {
        if(j->procs[i].pid == ret) {
          n--;
          j->procs[i].status = s;
        }
      }*/

      /* POSIX: pipeline exit status is the last command's status. job_fork's
         j->pgrp is unreliable (job_new pre-sets nproc to the pipeline width,
         so the `if(nproc==0) j->pgrp=pgrp` branch never fires), and using the
         pgrp leader's status would give the FIRST command's status anyway.
         Update on every reap; the last child to be reaped wins, which for a
         normal pipeline is the last command -- the only one whose stdin
         can only close after every upstream stage has finished writing. */
      *status = s;

      /* Non-interactive shells (scripts like ./configure) parse stderr to
         detect command behavior; the kernel often delivers SIGPIPE to the
         left side of a pipeline when the right side exits early, which is
         not an error worth printing. Keep the message in interactive (job
         control) mode and for signals other than SIGPIPE. */
      if(!WAIT_IF_EXITED(s)) {
        int squelch = !sh->opts.monitor && WAIT_IF_SIGNALED(s) &&
                      WAIT_TERMSIG(s) == SIGPIPE;
        if(!squelch)
          job_printstatus(ret, s);
      }
    }

    /* The "[id]+ Done command" job-completion banner is for interactive
       use only; suppress it in scripts so configure's stderr stays clean. */
    if(sh->opts.monitor) {
      char ch = job_current() == j ? '+' : (struct job*)job_pointer == j ? '-' : ' ';

      buffer_putc(fd_err->w, '[');
      buffer_putulong(fd_err->w, j->id);
      buffer_putc(fd_err->w, ']');
      buffer_putc(fd_err->w, ch);
      buffer_putspad(fd_err->w, "  Done", 26);
      buffer_puts(fd_err->w, j->command ? j->command : "(null)");
      buffer_putnlflush(fd_err->w);
    }

    if(job_done(j))
      job_free(j);

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
