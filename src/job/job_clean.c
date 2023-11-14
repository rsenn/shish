#include "../job.h"
#include "../../lib/sig.h"

void
job_clean(void) {
  struct job *j, *next;

  if(job_signaled) {
    job_signaled = 0;
    // sig_block(SIGCHLD);

    for(j = jobs; j; j = next) {
      next = j->next;

      if(job_done(j))
        job_free(j);
    }

    job_signaled = 0;
    // sig_unblock(SIGCHLD);
  }
}
