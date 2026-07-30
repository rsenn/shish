#include "../wait.h"
#include "../windoze.h"
#if WINDOWS_NATIVE
#include <windows.h>
#else
#include <sys/wait.h>
#endif

#include <errno.h>
#include <signal.h>

/* set by builtin_trap.c's trap_relay() (the real, async-signal-safe
 * OS handler for a real-signal trap) whenever a trapped signal fires;
 * cleared once trap_run_pending() has dispatched everything pending.
 * Checked below so an EINTR caused by a genuine trap signal doesn't
 * just get silently retried -- see waitpid_nointr()'s own comment. */
extern volatile sig_atomic_t trap_signaled;

int
waitpid_nointr(int pid, int* wstat, int flags) {
#if WINDOWS_NATIVE
  /* no project-defined equivalent of WNOHANG/WUNTRACED exists for
     Windows (see lib/wait.h) -- the only current caller, wait_pid(),
     always passes flags=0, so "flags" is accepted for interface
     parity but otherwise ignored here. There's also no real
     EINTR-equivalent to retry on: an un-alerted WaitForSingleObject()
     can't be interrupted the way a blocking unix syscall can. */
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
  } while((r == (int)-1) && (errno == EINTR) && !trap_signaled);

  return r;
#endif
}
