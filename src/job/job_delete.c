#include "../job.h"
#include "../../lib/alloc.h"
#include <assert.h>

void
job_delete(struct job* job) {

  for(struct job** jp = &jobs; *jp; jp = &(*jp)->next) {
    if(*jp == job) {
      *jp = job->next;
      alloc_free(job);
      return;
    }
  }
  // assert(0);
}
