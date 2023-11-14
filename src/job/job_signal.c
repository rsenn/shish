#include "../job.h"

/* signal job exiting
 * ----------------------------------------------------------------------- */
struct job*
job_signal(pid_t pid, int status) {
  struct job* job;

  if((job = job_bypid(pid))) {
    for(size_t i = 0; i < job->nproc; i++) {
      if(job->procs[i].pid == pid) {
        job->procs[i].status = status;
        return job;
      }
    }
  }

  return 0;
}
