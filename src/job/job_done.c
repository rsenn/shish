#include "../job.h"

/* is the job running
 * ----------------------------------------------------------------------- */
int
job_done(struct job* job) {
  for(size_t i = 0; i < job->nproc; i++)
    if(job->procs[i].status == -1)
      return 0;

  return 1;
}
