#include "../builtin.h"
#include "../exec.h"
#include "../job.h"
#include "../fdtable.h"
#include "../../lib/wait.h"

/* jobs builtin
 * ----------------------------------------------------------------------- */
int
builtin_jobs(int argc, char* argv[]) {

  struct job* job;

  for(job = jobs; job; job = job->next) {
    job_print(job, fd_out->w);
  }

  return 0;
}

/* fg builtin
 * ----------------------------------------------------------------------- */
int
builtin_fg(int argc, char* argv[]) {
  size_t njobs = argc - 1, jobindex = 0, npids = 0, k;
  struct job *job, *joblist[njobs];
  int *pidlist, status, r;

  if(argc == 1) {
    joblist[0] = job_current();
    njobs = 1;
  } else {
    for(int i = 1; i < argc; i++) {
      job = job_find(argv[i]);

      if(!job) {
        builtin_errmsg(argv, argv[i], "no such job");
        return 1;
      }

      joblist[jobindex++] = job;
    }
  }

  for(size_t j = 0; j < jobindex; j++) {
    npids += joblist[j]->nproc;
  }

  k = 0;
  pidlist = alloc(sizeof(int) * npids);

  for(size_t j = 0; j < jobindex; j++) {

    for(size_t l = 0; l < joblist[j]->nproc; l++) {
      pidlist[k++] = joblist[j]->procs[l].pid;
    }
  }

  while((r = wait_pids_nohang(pidlist, k, &status)) > 1) {}

  return 0;
}

/* bg builtin
 * ----------------------------------------------------------------------- */
int
builtin_bg(int argc, char* argv[]) {

  return 0;
}
