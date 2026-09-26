#include "../job.h"
#include "../term.h"
#include "../../lib/sig.h"
#include <unistd.h>
#include <assert.h>
#include "../trace.h"

void
job_foreground(struct job* job) {
  assert(job->pgrp > 0);

#if !WINDOWS_NATIVE
  TRACE(TRACE_JOB, "foreground", trace_int("id", job->id), trace_int("pgrp", job->pgrp));

  TRACE(TRACE_SIG, "block", trace_int("sig", SIGTTOU));
  sig_block(SIGTTOU);

  tcsetpgrp(term_input.fd, job->pgrp);

  TRACE(TRACE_SIG, "unblock", trace_int("sig", SIGTTOU));
  sig_unblock(SIGTTOU);
#endif
}
