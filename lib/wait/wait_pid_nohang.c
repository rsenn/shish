#include "../wait.h"
#include "../windoze.h"

#if WINDOWS_NATIVE
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#endif

int
wait_pid_nohang(int pid, int* wstat) {
#if WINDOWS_NATIVE
  DWORD exitcode = 0;
  HANDLE process = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION, FALSE, pid);

  if(!process)
    return -1;

  /* 0 timeout -- this is the *_nohang() variant, it must poll and
     return immediately instead of blocking until pid exits */
  if(WaitForSingleObject(process, 0) != WAIT_OBJECT_0) {
    CloseHandle(process);
    return 0;
  }

  GetExitCodeProcess(process, &exitcode);
  CloseHandle(process);
  wait_track_remove(pid);

  if(exitcode == STILL_ACTIVE)
    return 0;

  *wstat = exitcode;
  return pid;
#else
  int w = 0;
  int r = 0;

  while(r != pid) {
    r = wait_nohang(&w);

    if(!r || (r == (int)-1))
      return r;
  }
  *wstat = w;
  return r;
#endif
}
