#include "../../lib/alloc.h"
#include "../job.h"
#include <stdbool.h>

struct job *jobs = NULL, **jobptr;
volatile bool job_signaled = 0;

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

    for(int i = 0; i < job->nproc; i++) {
      job->procs[i].pid = 0;
      job->procs[i].status = -1;
    }

    job->next = *ptr;
    *ptr = job;
  }

  jobptr = ptr;

  return job;
}
