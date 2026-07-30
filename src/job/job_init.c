#include "../fdtable.h"
#include "../job.h"
#include <fcntl.h>
#include <string.h>
#include "../../lib/windoze.h"
#if !WINDOWS_NATIVE && !defined(__MINGW64__)
#include <termios.h>
#include <unistd.h>
#else
#include <io.h>
#endif

int job_terminal = -1;
int job_sigfd[2] = {-1, -1};

/* sets up job_terminal/job_pgrp once it's known whether stderr is
 * really a terminal -- split out of job_init() (see its call site in
 * sh_main.c) because that flag (FD_TERM) isn't set until term_init()
 * runs, which happens *after* job_init() does; calling this from
 * job_init() itself always saw a not-yet-updated fd_err->mode and left
 * job_terminal permanently at -1, silently disabling every
 * tcsetpgrp()-based terminal handoff in exec_program.c/job_fork.c/
 * job_wait.c even for a genuinely interactive session (BUGS:
 * job-terminal-never-initialized).
 * ----------------------------------------------------------------------- */
void
job_terminal_init(void) {
  struct fd* d;

  /* find a filedescriptor which is a terminal */
  if((d = fdtable[STDERR_FILENO]) && (fd_err->mode & FD_TERM)) {
#include "../../lib/windoze.h"
#if !WINDOWS_NATIVE && !defined(__MINGW64__)
    job_terminal = fcntl(d->e, F_DUPFD, 0x80);
#else
    job_terminal = dup(d->e);
#endif

#ifdef FD_CLOEXEC
    fcntl(job_terminal, F_SETFD, FD_CLOEXEC);
#endif
#if !WINDOWS_NATIVE
    job_pgrp = tcgetpgrp(job_terminal);
#endif
  }
}

/* initializes job control
 * ----------------------------------------------------------------------- */
void
job_init(void) {
#if !WINDOWS_NATIVE
  /* see job_sigfd's declaration in job.h -- sh_onsig() writes one
     byte per SIGCHLD, term_read() wakes on it via select() */
  if(pipe(job_sigfd) == 0) {
    int i;

    /* move both ends up above 0x80, same as job_terminal above --
       a raw pipe() hands back the two lowest-numbered free fds
       (typically 3 and 4 this early), which is exactly the range a
       script's own redirections use ("exec 3>&-", "9<in0 8<&9 ...").
       Leaving job_sigfd sitting on those collided with
       tests/posix/redir-p.tst's fd-chaining tests (hung, since a
       "3<&4"/"4<&5"-style dup could silently repoint or close a fd
       select() was still waiting on) -- confirmed by testing before
       this fix. */
    for(i = 0; i < 2; i++) {
      int hi = fcntl(job_sigfd[i], F_DUPFD, 0x80);

      if(hi != -1) {
        close(job_sigfd[i]);
        job_sigfd[i] = hi;
      }
    }

    for(i = 0; i < 2; i++) {
      int flags;

      if((flags = fcntl(job_sigfd[i], F_GETFL)) != -1)
        fcntl(job_sigfd[i], F_SETFL, flags | O_NONBLOCK);

      fcntl(job_sigfd[i], F_SETFD, FD_CLOEXEC);
    }
  }
#endif
}
