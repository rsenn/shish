#include "../job.h"
#include "../fdtable.h"
#include "../term.h"

void
job_clean(bool print) {
  struct job *j, *next;
  int erased = 0;

  for(j = job_list; j; j = next) {
    next = j->next;

    if(job_done(j)) {
      if(print) {
        /* whatever's on the current line (a prompt, in-progress
           typing) isn't ours to print over -- clear it and move to
           column 1 before the first banner, matching what
           sh_onsig()'s SIGCHLD handler already does before anything
           it prints. Once per call, not once per job. */
        if(!erased && term_output) {
          term_erase();
          erased = 1;
        }

        job_print(j, fd_err->w);
      }
      job_free(j);
    }
  }
}
