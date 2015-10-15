#include "shell.h"
#include "job.h"

struct job *job_list = NULL;
struct job **job_ptr = &job_list;

/* creates a new job structure
 * ----------------------------------------------------------------------- */
struct job *job_new(unsigned int n) {
  struct job *job;

  job = shell_alloc(sizeof(struct job) + sizeof(struct proc) * n);

  if(job) {
    job->next = NULL;
    job->procs = (struct proc *)&job[1];
    job->nproc = 0;
    job->pgrp = 0;

    *job_ptr = job;
    job_ptr = &job->next;
  }

  return job;
}

