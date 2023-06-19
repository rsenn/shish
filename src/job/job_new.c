#include "../../lib/alloc.h"
#include "../job.h"

struct job *jobs = NULL, **jobptr;

/* creates a new job structure
 * ----------------------------------------------------------------------- */
struct job*
job_new(unsigned int n) {
  struct job *job, **jptr = 0;

  job = alloc_zero(sizeof(struct job) + sizeof(struct proc) * n);

  if(job) {
    job->nproc = 0; // n;
    job->pgrp = 0;

    job->id = 1;
    for(jptr = &jobs; *jptr; jptr = &(*jptr)->next) {
      if((*jptr)->id != job->id)
        break;
      job->id++;
    }
    job->next = *jptr;
    *jptr = job;
  }

  jobptr = jptr;

  return job;
}
