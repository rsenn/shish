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
  WaitForSingleObject(process, INFINITE);
  GetExitCodeProcess(process, &exitcode);
  CloseHandle(process);

  if(exitcode == STILL_ACTIVE)
    return -1;
  return pid;
//#warning No windows implementation
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
