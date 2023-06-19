#include "../job.h"
#include "../../lib/alloc.h"
#include <assert.h>

void
job_delete(struct job* job) {
  struct job** jptr;

  for(jptr = &jobs; *jptr; jptr = &(*jptr)->next) {
    if(*jptr == job) {
      *jptr = job->next;
      alloc_free(job);
      return;
    }
  }
  // assert(0);
}
