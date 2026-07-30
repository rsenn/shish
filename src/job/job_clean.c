#include "../job.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../term.h"

void
job_clean(bool print) {
  struct job *j, *next;
  int erased = 0;

  for(j = job_list; j; j = next) {
    next = j->next;

    if(job_done(j)) {
      /* the "Done"/"Stopped"/etc. banner job_print() -> job_banner()
         prints is for interactive use only -- job_wait()'s own
         equivalent JOB_DONE banner is explicitly gated on
         sh->opts.monitor for exactly that reason ("so configure's
         stderr stays clean"), but this path (job_update(), called
         unconditionally every sh_loop() iteration whenever the async
         SIGCHLD handler beat job_wait() to reaping a job) printed it
         regardless of mode. A foreground job whose process gets
         reaped asynchronously before job_wait()'s own synchronous
         wait_pid() loop gets to it (a real race, not just
         theoretical -- e.g. a child killed by a signal while shish
         itself is between event-loop iterations) could have this path
         announce it before job_wait() ever runs its own gating, i.e. a
         stray "[1]+ Done ..." line in a plain non-interactive script's
         stderr. */
      if(print && sh->opts.monitor) {
        /* whatever's on the current line (a prompt, in-progress
           typing) isn't ours to print over -- clear it and move to
           column 1 before the first banner, matching what
           sh_onsig()'s SIGCHLD handler already does before anything
           it prints. Once per call, not once per job. */
        if(!erased && term_output) {
          term_erase();
          erased = 1;
        }

        job_print(j, fd_err->w);
      }
      job_free(j);
    } else if(print && sh->opts.monitor && job_stopped(j) && !j->announced) {
      /* a backgrounded job just stopped with nothing actively
         fg/bg/wait-ing on it to notice on its own -- announce it
         here, the same "[N]+ Stopped ..." line job_wait() prints
         when it catches a stop synchronously instead. Moved out of
         sh_onsig() (sh-onsig-async-unsafe, fixes/87): that handler
         isn't safe to do I/O from, so it now only records the status
         change and wakes term_read()'s select() loop, which calls
         job_update() -> here from ordinary context. j->announced
         keeps this from re-printing on every subsequent job_update()
         call while the job stays stopped -- job_resume() clears it
         again once the job actually resumes. */
      if(!erased && term_output) {
        term_erase();
        erased = 1;
      }

      job_banner(j, fd_err->w, JOB_STOPPED);
      j->announced = 1;
    }
  }
}
