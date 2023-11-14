#include "../../lib/alloc.h"
#include "../job.h"
#include <stdbool.h>

struct job *job_list = NULL, **job_pointer = NULL;
volatile bool job_signaled = 0;

/* creates a new job structure
 * ----------------------------------------------------------------------- */
struct job*
job_new(unsigned n) {
  struct job *j, **ptr = 0;

  j = alloc_zero(sizeof(struct job) + sizeof(struct proc) * n);

  if(j) {
    j->nproc = n;
    j->pgrp = 0;
    j->id = 1;

    for(ptr = &job_list; *ptr; ptr = &(*ptr)->next) {
      if((*ptr)->id != j->id)
        break;

      j->id++;
    }

    for(int i = 0; i < j->nproc; i++) {
      j->procs[i].pid = 0;
      j->procs[i].status = -1;
    }

    j->next = *ptr;
    *ptr = j;
  }

  job_pointer = ptr;

  return j;
}
