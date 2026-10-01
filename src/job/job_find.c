#include "../job.h"
#include "../sh.h"
#include "../../lib/scan.h"
#include "../../lib/str.h"

/* find a job by job ID ("%%" "%+" "%-" "%n" "%string" "%?string") or by PID
 * ----------------------------------------------------------------------- */
struct job*
job_find(const char* str) {
  struct job* job = 0;
  unsigned int id = 0;

  if(*str == '%') {
    const char* spec = str + 1;
    size_t len = str_len(spec);

    /* "%", "%%", "%+": the current job; "%-": the one before it */
    if(!len || !str_diff(spec, "%") || !str_diff(spec, "+"))
      return job_current();

    if(!str_diff(spec, "-")) {
      struct job* prev = 0;

      for(job = job_list; job; job = job->next)
        if(job != job_current() && job->level == sh_subshell)
          prev = job;

      return prev;
    }

    if(scan_uint(spec, &id) == len) {
      for(job = job_list; job; job = job->next)
        if(job->id == (int)id && job->level == sh_subshell)
          break;

      return job;
    }

    /* "%?string": the command contains string; "%string": it starts with it */
    for(job = job_list; job; job = job->next) {
      const char* cmd = job->command ? job->command : "";

      if(job->level != sh_subshell)
        continue;

      if(*spec == '?') {
        size_t n = len - 1, i, clen = str_len(cmd);

        for(i = 0; i + n <= clen; i++)
          if(!str_diffn(cmd + i, spec + 1, n))
            return job;
      } else if(!str_diffn(cmd, spec, len)) {
        return job;
      }
    }

    return 0;
  }

  scan_uint(str, &id);

  if(id) {
    for(job = job_list; job; job = job->next) {
      size_t i;

      for(i = 0; i < job->nproc && job->level == sh_subshell; i++)
        if(job->procs[i].pid == (pid_t)id)
          return job;
    }
  }

  return 0;
}

struct job*
job_first(void) {
  struct job* job;

  for(job = job_list; job; job = job->next)
    if(job->level == sh_subshell)
      return job;

  return 0;
}

void
job_discard(int level) {
  struct job *job, *next;

  for(job = job_list; job; job = next) {
    next = job->next;

    if(job->level > level)
      job_free(job);
  }
}
