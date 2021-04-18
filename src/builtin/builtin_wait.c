#include "../builtin.h"
#include "../exec.h"
#include "../job.h"
#include "../../lib/typedefs.h"

/* command built-in
 *
 * ----------------------------------------------------------------------- */
int
builtin_wait(int argc, char* argv[]) {
  size_t i, njobs;
  struct job* jobs[argc];

  njobs = argc - 1;

  for(i = 0; i < njobs; i++) {
    if(!(jobs[i] = job_find(argv[i + 1]))) {
      builtin_errmsg(argv, argv[i + 1], "no such job");
      return 1;
    }
  }

  for(i = 0; i < njobs; i++) {
    int status;
    job_wait(jobs[i], 0, &status);
  }

  return 0;
}
