#if defined(DEBUG_OUTPUT) && (defined(DEBUG_JOB))
#include "../../lib/buffer.h"
#include "../debug.h"
#include "../job.h"

void
job_dump(buffer* b) {
  struct job* job = 0;

  if(job_list)
    buffer_puts(b,
                "  id     pgrp   command      "
                "        "
                " done PIDs\n");

  for(job = job_list; job; job = job->next) {
    int current = job_current() == job;

    buffer_puts(b, current ? "\033[1;33m " : " ");
    buffer_putlong0(b, job->id, 3);
    buffer_putlong0(b, job->pgrp, 9);
    buffer_putc(b, '\t');
    buffer_putspad(b, job->command, 22);
    buffer_putspad(b, job_done(job) ? "🞩" : "–", 6);

    for(int i = 0; i < job->nproc; i++) {
      buffer_putc(b, ' ');
      buffer_putlong0(b, job->procs[i].pid, 3);
    }

    if(current)
      buffer_puts(b, "\033[0m");

    buffer_putnlflush(b);
  }
}
#endif /* DEBUG_OUTPUT */
