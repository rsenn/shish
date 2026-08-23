#include "../windoze.h"

#if WINDOWS_NATIVE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sys/types.h>
#include <errno.h>
#include "../sig.h"

/* Windows has no per-signal disposition to deliver to, so kill() can
 * only approximate a handful of POSIX signals by their closest real
 * effect -- and must fail honestly (ESRCH/ENOSYS), not fake success,
 * for everything else. See doc/building.md's Windows section.
 *
 *   SIGKILL, SIGTERM   OpenProcess + TerminateProcess (ungraceful)
 *   SIGINT             GenerateConsoleCtrlEvent(CTRL_C_EVENT, pid) --
 *                       only works if 'pid' is a real console process
 *                       group id (CREATE_NEW_PROCESS_GROUP); shish
 *                       doesn't create children that way yet, so this
 *                       currently just fails for ordinary pids
 *   0                  existence check only, no signal sent
 *   anything else      ENOSYS -- no Windows analog exists */
int
kill(pid_t pid, int sig) {
  HANDLE h;

  switch(sig) {
  case SIGKILL:
  case SIGTERM:
    if(!(h = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD)pid)))
      return (errno = ESRCH, -1);

    if(!TerminateProcess(h, 128 + sig)) {
      CloseHandle(h);
      return (errno = EPERM, -1);
    }

    CloseHandle(h);
    return 0;

  case SIGINT:
    if(!GenerateConsoleCtrlEvent(CTRL_C_EVENT, (DWORD)pid))
      return (errno = ESRCH, -1);

    return 0;

  case 0:
    if(!(h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (DWORD)pid)))
      return (errno = ESRCH, -1);

    CloseHandle(h);
    return 0;

  default:
    return (errno = ENOSYS, -1);
  }
}
#endif
