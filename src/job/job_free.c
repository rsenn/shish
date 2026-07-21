#include "../job.h"
#include "../../lib/alloc.h"

static void
job_delete(struct job** j) {
  struct job* next = (*j)->next;
  int was_current = (j == job_pointer);

  alloc_free(*j);
  *j = next;

  if(was_current) {
    /* promote the previous job to current ("%-" becoming the new
       "%+", as bash does) instead of just losing track of "current"
       once it's gone. job_list is kept in ascending-id order (see
       job_new()), so the last surviving entry is the most-recently-
       created still-alive job -- nothing here tracks "%-" more
       precisely than that, but it's the same job job_new() would
       have made current next anyway. */
    struct job** p = &job_list;

    while(*p && (*p)->next)
      p = &(*p)->next;

    job_pointer = *p ? p : NULL;
  }
}

void
job_free(struct job* job) {
  for(struct job** jp = &job_list; *jp; jp = &(*jp)->next)
    if(*jp == job) {
      job_delete(jp);
      break;
    }
}
