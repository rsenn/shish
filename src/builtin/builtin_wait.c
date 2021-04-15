#include "../builtin.h"
#include "../exec.h"
#include "../job.h"

/* command built-in
 *
 * ----------------------------------------------------------------------- */
int
builtin_wait(int argc, char* argv[]) {
  size_t i;
  struct job* job;
  for(i = 1; i < argc; i++) {
    if(!(job = job_find(argv[i]))) {
      builtin_errmsg(argv, argv[i], "no such job");
      return 1;
    }
  }

  return 0;
}
