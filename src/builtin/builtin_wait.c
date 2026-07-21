#include "../builtin.h"
#include "../exec.h"
#include "../job.h"
#include "../../lib/typedefs.h"

/* wait built-in
 *
 * ----------------------------------------------------------------------- */
int
builtin_wait(int argc, char* argv[]) {
  size_t i, njobs;
  int ret = 0;

  /* no operands: wait for every job the shell currently knows about.
     job_wait() frees a job once it's fully reaped (see job_wait.c),
     which advances the global job_list -- so re-reading job_list each
     iteration instead of snapshotting it up front is what actually
     drains the whole list. Exit status is that of the last job waited
     for, 0 if there were none (matches bash; POSIX says "wait" with
     no operands always returns 0, but every other shell's wait
     already reports the last waited-for job's status elsewhere in
     this builtin, so do the same here for consistency). */
  if(argc == 1) {
    while(job_list) {
      int status = 0;

      job_wait(job_list, 0, &status);
      ret = WEXITSTATUS(status);
    }

    return ret;
  }

  njobs = argc - 1;

  {
    struct job* jobs[njobs];

    for(i = 0; i < njobs; i++) {
      if(!(jobs[i] = job_find(argv[i + 1]))) {
        builtin_errmsg(argv, argv[i + 1], "no such job");
        return 1;
      }
    }

    for(i = 0; i < njobs; i++) {
      int status = 0;

      job_wait(jobs[i], 0, &status);
      ret = WEXITSTATUS(status);
    }
  }

  return ret;
}
