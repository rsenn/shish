#include "../windoze.h"

#if WINDOWS_NATIVE
#include <sys/types.h>

int kill(pid_t pid, int sig);

/* No real process groups exist on Windows yet (job control's own
 * setpgid()/tcsetpgrp() are unimplemented there -- see
 * BUGS:mingw-missing-tcsetpgrp) -- 'pgrp' is only ever the group
 * leader's own pid (job->pgrp in job_fork.c), so the best available
 * approximation is signaling that one process, not the whole group. */
int
killpg(pid_t pgrp, int sig) {
  return kill(pgrp, sig);
}
#endif
