#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "../../lib/windoze.h"
#if !WINDOWS_NATIVE
#include <termios.h>
#include <unistd.h>
#endif
#include "../fdtable.h"
#include "../job.h"
#include "../sh.h"
#include "../../lib/wait.h"

/* waits for a job to terminate
 * ----------------------------------------------------------------------- */
int
job_wait(struct job* job, pid_t pid, int* status) {
  int ret = 0;

  if(job) {
    unsigned int n = job->nproc;

    while(n > 0) {
      unsigned int i;
      int st; /* status */

      if((ret = wait_pid(job->pgrp ? -job->pgrp : job->procs[0].pid, &st)) <= 0)
        break;

      if(job_signal(ret, st))
        n--;

      /*for(i = 0; i < job->nproc; i++) {
        if(job->procs[i].pid == ret) {
          n--;
          job->procs[i].status = st;
        }
      }*/

      if(ret == job->pgrp)
        *status = st;

      job_status(ret, st);
    }

    char ch = job_current() == job ? '+' : (struct job*)jobptr == job ? '-' : ' ';

    buffer_putc(fd_err->w, '[');
    buffer_putulong(fd_err->w, job->id);
    buffer_putc(fd_err->w, ']');
    buffer_putc(fd_err->w, ch);
    buffer_putspad(fd_err->w, "  Done", 26);
    buffer_puts(fd_err->w, job->command ? job->command : "(null)");
    buffer_putnlflush(fd_err->w);

    job->done = 1;

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

  if(job && job->done)
    job_delete(job);

  return ret;
}
