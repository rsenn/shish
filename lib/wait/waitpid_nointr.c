#include "../wait.h"
#include "../windoze.h"
#if WINDOWS_NATIVE
#include <windows.h>
#else
#include <sys/wait.h>
#endif

#include <errno.h>

int
waitpid_nointr(int pid, int* wstat, int flags) {
#if WINDOWS_NATIVE
  /* no project-defined equivalent of WNOHANG/WUNTRACED exists for
     Windows (see lib/wait.h) -- the only current caller,
     wait_pid.c's non-Windows branch, always passes flags=0, so
     "flags" is accepted for interface parity but otherwise ignored
     here, same as wait_pid()'s own Windows branch a few lines over.
     There's also no real EINTR-equivalent to retry on: an
     un-alerted WaitForSingleObject() can't be interrupted the way a
     blocking unix syscall can. */
  HANDLE hproc = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION, FALSE, pid);
  DWORD exitcode = 0;

  (void)flags;

  if(!hproc)
    return -1;

  if(WaitForSingleObject(hproc, INFINITE) != WAIT_OBJECT_0) {
    CloseHandle(hproc);
    return -1;
  }

  GetExitCodeProcess(hproc, &exitcode);
  CloseHandle(hproc);
  wait_track_remove(pid);

  if(exitcode == STILL_ACTIVE)
    return -1;

  *wstat = exitcode;
  return pid;
#else
  int r;

  do {
    r = waitpid(pid, wstat, flags);
  } while((r == (int)-1) && (errno == EINTR));

  return r;
#endif
}
