#include "../wait.h"
#include "../windoze.h"

#if !WINDOWS_NATIVE
#include <sys/types.h>
#include <sys/wait.h>
#endif

/* like wait_nohang(), but also reports a child that just stopped
 * (Ctrl-Z / SIGTSTP, or an explicit "kill -STOP"), not only ones that
 * exited -- needed so the SIGCHLD handler can notice a backgrounded
 * job stopping even though nothing is actively fg/bg/wait-ing on it
 * at that moment (job_wait()'s own wait_pid_untraced() only covers
 * the case where something *is*). No WINDOWS_NATIVE equivalent
 * exists (there's no POSIX-style job-control stop state there), so it
 * just behaves like wait_nohang().
 * ----------------------------------------------------------------------- */
int
wait_nohang_untraced(int* wstat) {
#if WINDOWS_NATIVE
  return wait_nohang(wstat);
#elif defined(__unix__)
  return waitpid(-1, wstat, WNOHANG | WUNTRACED);
#else
  return wait_nohang(wstat);
#endif
}
