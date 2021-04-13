#include "../../lib/alloc.h"
#include "../job.h"

/* creates a new job structure
 * ----------------------------------------------------------------------- */
void
job_delete(struct job* job) {
  struct job** jp;

  for(jp = &jobs; *jp; jp = &(*jp)->next) {
    if(*jp == job) {

      *jp = job->next;
      alloc_free(job);
      break;
    }
  }
}
