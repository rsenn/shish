#include "../job.h"

/* signal job exiting
 * ----------------------------------------------------------------------- */
struct job*
job_signal(pid_t pid, int status) {
  struct job* j;

  if((j = job_bypid(pid))) {
    for(size_t i = 0; i < j->nproc; i++) {
      struct proc* p = &j->procs[i];

      if(p->pid == pid) {
        p->status = status;

        job_signaled = true;
        return j;
      }
    }
  }

  return 0;
}
