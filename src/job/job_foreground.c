#include "../job.h"
#include "../term.h"
#include "../../lib/sig.h"
#include <unistd.h>
#include <assert.h>

void
job_foreground(struct job* job) {
  assert(job->pgrp > 0);

  sig_block(SIGTTOU);

  tcsetpgrp(term_input.fd, job->pgrp);

  sig_unblock(SIGTTOU);
}
