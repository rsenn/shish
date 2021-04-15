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
    for(job = jobs; job; job->next) {

      if(*str == '%') {
        if(job->id == id)
          break;
      } else {
        size_t i;
        for(i = 0; i < job->nproc; i++)
          if(job->procs[i].pid == id)
            break;
      }
    }
  }
  return job;
}
