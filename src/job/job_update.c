#include "../job.h"
#include "../../lib/sig.h"
#include "../trace.h"

void
job_update(void) {
  struct job *j, *next;

  if(job_signaled) {
    TRACE(TRACE_JOB, "update");
    TRACE(TRACE_SIG, "block", trace_int("sig", SIGCHLD));
    sig_block(SIGCHLD);

    job_clean(true);

    job_signaled = 0;
    TRACE(TRACE_SIG, "unblock", trace_int("sig", SIGCHLD));
    sig_unblock(SIGCHLD);
  }
}
