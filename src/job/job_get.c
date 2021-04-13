#include "../../lib/alloc.h"
#include "../job.h"

/* creates a new job structure
 * ----------------------------------------------------------------------- */
struct job*
job_get(int id) {
  struct job* job;

  for(job = jobs; job; job = job->next)
    if(job->id == id)
      return job;
  return 0;
}
