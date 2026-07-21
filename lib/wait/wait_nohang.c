#include "../wait.h"
#include "../windoze.h"

#if WINDOWS_NATIVE
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#endif

int
wait_nohang(int* wstat) {
#if WINDOWS_NATIVE
  unsigned int i, n = wait_track_count();

  /* no children left to wait for -- mirrors POSIX waitpid(-1, ...)
   * returning -1/ECHILD in the same situation */
  if(n == 0)
    return -1;

  for(i = 0; i < n; i++) {
    int pid = wait_track_pid(i);
    HANDLE hproc = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION, FALSE, pid);
    DWORD exitcode = 0;

    if(!hproc) {
      /* the process object is gone without ever being reaped here
       * (reaped directly via wait_pid()/wait_pid_nohang() instead) --
       * drop it so we stop trying */
      wait_track_remove(pid);
      continue;
    }

    if(WaitForSingleObject(hproc, 0) == WAIT_OBJECT_0) {
      GetExitCodeProcess(hproc, &exitcode);
      CloseHandle(hproc);
      wait_track_remove(pid);
      *wstat = exitcode;
      return pid;
    }

    CloseHandle(hproc);
  }

  /* children exist, but none of them have exited yet -- mirrors
   * POSIX waitpid(-1, ..., WNOHANG) returning 0 in the same
   * situation */
  return 0;
#elif defined(__unix__)
  return waitpid(-1, wstat, WNOHANG);
#else
  return wait3(wstat, WNOHANG, (struct rusage*)0);
#endif
}
