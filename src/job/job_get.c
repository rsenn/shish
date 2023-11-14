#include "../../lib/alloc.h"
#include "../job.h"

/* creates a new job structure
 * ----------------------------------------------------------------------- */
struct job*
job_get(int id) {
  for(struct job* job = job_list; job; job = job->next)
    if(job->id == id)
      return job;

  return 0;
}
