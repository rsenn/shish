#include "../../lib/alloc.h"
#include "../job.h"
#include "../../lib/alloc.h"

struct job* job_list = NULL;
struct job** job_ptr = &job_list;

/* creates a new job structure
 * ----------------------------------------------------------------------- */
struct job*
job_new(unsigned int n) {
  struct job* job;

  job = alloc_zero(sizeof(struct job) + sizeof(struct proc) * n);

  if(job) {
    job->next = NULL;
     job->nproc = n;
    job->pgrp = 0;

    *job_ptr = job;
    job_ptr = &job->next;
  }

  return job;
}
