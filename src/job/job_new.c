#include "../../lib/alloc.h"
#include "../job.h"

struct job *jobs = NULL, **jobptr;

/* creates a new job structure
 * ----------------------------------------------------------------------- */
struct job*
job_new(unsigned int n) {
  struct job *job, **ptr = 0;

  job = alloc_zero(sizeof(struct job) + sizeof(struct proc) * n);

  if(job) {
    job->nproc = n;
    job->pgrp = 0;
    job->id = 1;

    for(ptr = &jobs; *ptr; ptr = &(*ptr)->next) {
      if((*ptr)->id != job->id)
        break;

      job->id++;
    }

    job->next = *ptr;
    *ptr = job;
  }

  jobptr = ptr;

  return job;
}
