#include "../builtin.h"
#include "../exec.h"
#include "../job.h"
#include "../../lib/typedefs.h"
#include "../../lib/wait.h"
#include "../../lib/scan.h"

/* wait built-in
 *
 * ----------------------------------------------------------------------- */
const char help_wait[] = "    Wait for background jobs to finish.\n"
                         "\n"
                         "    pid             job/process to wait for; default is every job\n"
                         "                    the shell currently knows about\n";

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
    struct job* j;

    while((j = job_first())) {
      int status = 0;

      job_wait_interruptible = 1;
      job_wait_sig = 0;
      job_wait(j, 0, &status);
      job_wait_interruptible = 0;

      if(job_wait_sig)
        return 128 + job_wait_sig; /* a trap ran: POSIX 2.9.3.1 */

      ret = WAIT_STATUS(status);
    }

    return ret;
  }

  njobs = argc - 1;

  {
    struct job* jobs[njobs];

    for(i = 0; i < njobs; i++)
      if(!(jobs[i] = job_find(argv[i + 1])) && argv[i + 1][0] == '%')
        builtin_errmsg(argv, argv[i + 1], "no such job");

    /* status is that of the last operand; an unknown one counts as 127 */
    for(i = 0; i < njobs; i++) {
      int status = 0;

      if(!jobs[i]) {
        unsigned int pid = 0;
        int st;

        /* a pid whose job has already finished and been cleaned up */
        if(argv[i + 1][0] != '%' && scan_uint(argv[i + 1], &pid) && job_recall(pid, &st)) {
          ret = WAIT_STATUS(st);
          continue;
        }

        ret = 127;
        continue;
      }

      job_wait_interruptible = 1;
      job_wait_sig = 0;
      job_wait(jobs[i], 0, &status);
      job_wait_interruptible = 0;

      if(job_wait_sig)
        return 128 + job_wait_sig; /* a trap ran: POSIX 2.9.3.1 */

      ret = WAIT_STATUS(status);
    }
  }

  return ret;
}
