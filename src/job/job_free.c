#include "../job.h"
#include "../../lib/alloc.h"

/* exit statuses of recently finished jobs' processes, so "wait PID" still
 * reports one after the job itself is gone (POSIX 2.9.3.1) */
static struct {
  pid_t pid;
  int status;
} job_finished[64];
static unsigned job_finished_n;

int
job_recall(pid_t pid, int* status) {
  unsigned i;

  for(i = 0; i < 64; i++)
    if(job_finished[i].pid == pid && pid > 0) {
      *status = job_finished[i].status;
      return 1;
    }

  return 0;
}

static void
job_delete(struct job** j) {
  struct job* next = (*j)->next;
  int was_current = (j == job_pointer);
  size_t i;

  /* only a job started with "&": one that ran in the foreground (fg) is gone for good */
  for(i = 0; (*j)->bgnd && i < (*j)->nproc; i++) {
    job_finished[job_finished_n % 64].pid = (*j)->procs[i].pid;
    job_finished[job_finished_n % 64].status = (*j)->procs[i].status;
    job_finished_n++;
  }

  /* job_pointer is the *slot* holding the current job: when that slot is the
     "next" field of the job being freed, the slot moves to where it was linked */
  if(job_pointer == &(*j)->next)
    job_pointer = j;

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
