#include "../wait.h"
#include "../windoze.h"
#if !WINDOWS_NATIVE
#include <sys/wait.h>
#endif

/* like wait_pid(), but also returns when the child stops (e.g.
 * Ctrl-Z / SIGTSTP) instead of only when it exits -- job_wait() needs
 * this to notice a foreground job getting stopped rather than
 * blocking until it eventually exits. No WINDOWS_NATIVE equivalent
 * exists (there's no POSIX-style job-control stop state to wait for
 * there), so it just behaves like wait_pid().
 * ----------------------------------------------------------------------- */
int
wait_pid_untraced(int pid, int* wstat) {
#if WINDOWS_NATIVE
  return wait_pid(pid, wstat);
#else
  return waitpid_nointr(pid, wstat, WUNTRACED);
#endif
}
