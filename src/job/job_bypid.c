#include "../job.h"

/* find a job by PID
 * ----------------------------------------------------------------------- */
struct job*
job_bypid(pid_t pid) {

  for(struct job* job = job_list; job; job = job->next)
    for(size_t i = 0; i < job->nproc; i++)
      if(job->procs[i].pid == pid)
        return job;

  return 0;
}
