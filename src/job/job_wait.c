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

      if(ret == j->pgrp)
        *status = s;

      job_status(ret, s);
    }

    char ch = job_current() == j ? '+' : (struct job*)jobptr == j ? '-' : ' ';

    buffer_putc(fd_err->w, '[');
    buffer_putulong(fd_err->w, j->id);
    buffer_putc(fd_err->w, ']');
    buffer_putc(fd_err->w, ch);
    buffer_putspad(fd_err->w, "  Done", 26);
    buffer_puts(fd_err->w, j->command ? j->command : "(null)");
    buffer_putnlflush(fd_err->w);

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
