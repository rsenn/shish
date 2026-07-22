#include "../../lib/buffer.h"
#include "../job.h"

/* every line the shell prints to tell the user something happened to
 * a job -- "[id] pid" when one is freshly backgrounded
 * (eval_node_bgnd.c, exec_command.c, exec_program.c), "[id]+ Done
 * ..."/"[id]+ Stopped ..." once job_wait() (synchronously) or
 * sh_onsig() (asynchronously, via SIGCHLD) notices it finished or
 * stopped, "[id]+ command &" when "bg" resumes a stopped one, and the
 * "[id]+ Status command" line "jobs" lists each job with (job_print()
 * below) -- used to each hand-roll their own buffer_put*() calls,
 * which had already drifted out of sync once (job_wait()'s banners
 * padded to a different column than job_print()'s, though they came
 * out the same width by coincidence; the "current job" marker had
 * three subtly different implementations). One formatter now, so
 * there's exactly one place left to get it right.
 * ----------------------------------------------------------------------- */
void
job_banner(struct job* job, buffer* out, enum job_banner_kind kind) {
  buffer_putc(out, '[');
  buffer_putulong(out, job->id);
  buffer_putc(out, ']');

  if(kind == JOB_START) {
    /* no status/current-job marker for this one -- it's just "here's
       the job id and pid you'll want for a later fg/bg/wait/kill" */
    buffer_putc(out, ' ');
    buffer_putulong(out, job->procs[0].pid);
    buffer_putnlflush(out);
    return;
  }

  buffer_putc(out, job_current() == job ? '+' : ' ');

  switch(kind) {
    case JOB_RUNNING: buffer_putspad(out, "  Running", 26); break;
    case JOB_DONE: buffer_putspad(out, "  Done", 26); break;
    case JOB_STOPPED: buffer_putspad(out, "  Stopped", 26); break;
    case JOB_BGRESUME: buffer_putc(out, ' '); break;
    default: break;
  }

  buffer_puts(out, job->command ? job->command : "(null)");

  if(kind == JOB_BGRESUME)
    buffer_puts(out, " &");

  buffer_putnlflush(out);
}
