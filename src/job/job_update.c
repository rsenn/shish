#include "../job.h"
#include "../../lib/sig.h"
#include "../trace.h"

void
job_update(void) {
  struct job *j, *next;

  if(job_signaled) {
    TRACE(TRACE_JOB, "update");
    sig_block(SIGCHLD);

    job_clean(true);

    job_signaled = 0;
    sig_unblock(SIGCHLD);
  }
}
