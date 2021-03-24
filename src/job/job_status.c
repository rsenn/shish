#include "../../lib/windoze.h"
#include "../fd.h"
#include "../job.h"
#include "../../lib/wait.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* outputs job status stuff
 * ----------------------------------------------------------------------- */
void
job_status(int pid, int status) {
  if(WAIT_IF_SIGNALED(status)) {
    const char* signame = sys_siglist[WAIT_TERMSIG(status)];

    if(pid) {
      buffer_put(fd_err->w, "process ", 8);
      buffer_putulong(fd_err->w, pid);
      buffer_put(fd_err->w, " signaled: ", 11);
    }

    buffer_putc(fd_err->w, signame[0] + 0x20);
    buffer_puts(fd_err->w, &signame[1]);

#ifdef WCOREDUMP
    if(WCOREDUMP(status)) {
      buffer_putspace(fd_err->w);
      buffer_put(fd_err->w, "(core dumped)", 13);
    }
#endif /* WCOREDUMP */

    buffer_putnlflush(fd_err->w);

    exit(666);
  }
}
