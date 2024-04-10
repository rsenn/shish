#include "../job.h"
#include "../fdtable.h"

void
job_clean(bool print) {
  struct job *j, *next;

  for(j = job_list; j; j = next) {
    next = j->next;

    if(job_done(j)) {
      if(print)
        job_print(j, fd_err->w);
      job_free(j);
    }
  }
}
