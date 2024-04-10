#include "../../lib/windoze.h"
#include "../fdtable.h"
#include "../job.h"
#include "../../lib/wait.h"
#include "../../lib/sig.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* outputs job status stuff
 * ----------------------------------------------------------------------- */
void
job_status(pid_t pid, int status) {

  if(WAIT_IF_EXITED(status)) {
    buffer_put(fd_err->w, "process ", 8);
    buffer_putulong(fd_err->w, pid);
    buffer_puts(fd_err->w, " exited: ");
    buffer_putulong(fd_err->w, WAIT_EXITSTATUS(status));
    buffer_putnlflush(fd_err->w);
  }

  if(WAIT_IF_STOPPED(status)) {
    buffer_put(fd_err->w, "process ", 8);
    buffer_putulong(fd_err->w, pid);
    buffer_putsflush(fd_err->w, " stopped\n");
  }

  if(WAIT_IF_SIGNALED(status)) {
    const char* signame = sig_name(WAIT_TERMSIG(status));

    if(pid) {
      buffer_put(fd_err->w, "process ", 8);
      buffer_putulong(fd_err->w, pid);
      buffer_put(fd_err->w, " signaled: ", 11);
    }

    // buffer_putc(fd_err->w, signame[0] + 0x20);
    buffer_puts(fd_err->w, signame);

#ifdef WCOREDUMP
    if(WCOREDUMP(status)) {
      buffer_putspace(fd_err->w);
      buffer_put(fd_err->w, "(core dumped)", 13);
    }
#endif /* WCOREDUMP */

    buffer_putnlflush(fd_err->w);

    // exit(666);
  }
}
