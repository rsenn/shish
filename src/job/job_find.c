#include "../job.h"
#include "../../lib/scan.h"

/* find a job by job-id or PID
 * ----------------------------------------------------------------------- */
struct job*
job_find(const char* str) {
  struct job* job = 0;
  unsigned int id = 0;

  scan_uint(*str == '%' ? str + 1 : str, &id);

  if(id) {
    for(job = job_list; job; job = job->next) {

      if(*str == '%') {
        if(job->id == id)
          break;

      } else {
        size_t i;
        int found = 0;

        for(i = 0; i < job->nproc; i++)
          if(job->procs[i].pid == id) {
            found = 1;
            break;
          }

        /* the inner loop's break above only ended the pid scan
           within this one job; without checking "found" here too,
           the outer loop kept walking past a real match to the next
           job regardless, so a bare-pid lookup (no "%" prefix, e.g.
           "wait $!") never actually found anything */
        if(found)
          break;
      }
    }
  }

  return job;
}
