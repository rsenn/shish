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
#include "../term.h"
#include "../../lib/wait.h"
#include <signal.h>

/* waits for a job to terminate
 * ----------------------------------------------------------------------- */
int
job_wait(struct job* j, pid_t pid, int* status) {
  int ret = 0, s;
  size_t i;
  /* bail-out bound for the retry loop below, in case wait_pid() and
     the async SIGCHLD handler both keep finding nothing (e.g. an
     unrelated bug loses track of a process entirely) -- 5000 * 1ms is
     ~5s, generous for a real race but short enough not to hang the
     shell indefinitely on a genuine bug elsewhere */
  int spins = 5000;

  if(j) {
    for(;;) {
      size_t remaining = 0;

      /* the SIGCHLD handler (sh_onsig() -> wait_nohang() ->
         job_signal()) can race ahead of us and reap a process on its
         own -- a real race for a child that exits fast, e.g. "cmd &"
         immediately followed by "wait": the child can be long gone
         (and its status already recorded here via job_signal(), same
         as our own reap below does) before we ever get to try
         wait_pid() ourselves. Recompute what's still outstanding from
         procs[].status on every pass, rather than trying to reap
         everything ourselves and losing whatever the handler already
         got to first (confirmed: "cmd & wait; echo $?" always printed
         0 this way, regardless of cmd's real exit status). */
      for(i = 0; i < j->nproc; i++) {
        if(j->procs[i].status != -1)
          *status = j->procs[i].status;
        else
          remaining++;
      }

      if(remaining == 0)
        break;

      ret = wait_pid(j->pgrp ? -j->pgrp : j->procs[0].pid, &s);

      if(ret > 0) {
        job_signal(ret, s);
        *status = s;

        /* Non-interactive shells (scripts like ./configure) parse
           stderr to detect command behavior; the kernel often
           delivers SIGPIPE to the left side of a pipeline when the
           right side exits early, which is not an error worth
           printing. Keep the message in interactive (job control)
           mode and for signals other than SIGPIPE. */
        if(!WAIT_IF_EXITED(s)) {
          int squelch = !sh->opts.monitor && WAIT_IF_SIGNALED(s) &&
                        WAIT_TERMSIG(s) == SIGPIPE;
          if(!squelch)
            job_printstatus(ret, s);
        }
      } else {
        /* wait_pid() found nothing left to reap itself -- either the
           SIGCHLD handler beat us to everything (loop back around;
           the scan above will pick up what it recorded) or there
           really is nothing left. Either way, avoid spinning a tight
           CPU-bound loop against an async handler that hasn't run
           yet: give it a moment. */
        if(--spins <= 0)
          break;

        usleep(1000);
      }
    }

    /* The "[id]+ Done command" job-completion banner is for interactive
       use only; suppress it in scripts so configure's stderr stays clean.
       It's also only meaningful for a job that was actually backgrounded
       ("cmd &") -- a foreground pipeline (including one run internally
       just to capture a command substitution's output, which never had
       a ->command string set at all, hence "(null)") isn't something the
       user asked to be notified about finishing; they were already
       watching it, or for a substitution, never saw it as a job in the
       first place. */
    if(sh->opts.monitor && j->bgnd) {
      char ch = job_current() == j ? '+' : (struct job*)job_pointer == j ? '-' : ' ';

      /* whatever's on the current line (a prompt, in-progress typing)
         isn't ours to print over -- clear it and move to column 1
         first, matching what sh_onsig()'s SIGCHLD handler already does
         before anything it prints */
      if(term_output)
        term_erase();

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
