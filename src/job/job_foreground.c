#include "../job.h"
#include "../../lib/sig.h"

void
job_foreground(struct job * job) {
  assert(job->pgrp > 0);

sig_block(SIGTTOU);

  tcsetpgrp(ttyfd, pgrp);

sig_unblock(SIGTTOU);
}
