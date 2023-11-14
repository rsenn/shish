#include "../job.h"
#include "../../lib/alloc.h"

static void
job_delete(struct job** j) {
  struct job* next = (*j)->next;

  alloc_free(*j);

  if(j == jobptr)
    jobptr = NULL;

  *j = next;
}

void
job_free(struct job* job) {
  for(struct job** jp = &jobs; *jp; jp = &(*jp)->next)
    if(*jp == job) {
      job_delete(jp);
      break;
    }
}
