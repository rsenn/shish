#include "../../lib/buffer.h"
#include "../job.h"

/* ----------------------------------------------------------------------- */
void
job_print(struct job* job, buffer* out) {
  job_banner(job, out, job_done(job) ? JOB_DONE : job_stopped(job) ? JOB_STOPPED : JOB_RUNNING);
}
