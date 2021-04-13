#include "../builtin.h"
#include "../exec.h"
#include "../job.h"
#include "../fdtable.h"

/* command built-in
 *
 * ----------------------------------------------------------------------- */
int
builtin_jobs(int argc, char* argv[]) {

  struct job* job;

  for(job = jobs; job; job = job->next) {
    job_print(job, fd_out->w);
  }

  return 0;
}
