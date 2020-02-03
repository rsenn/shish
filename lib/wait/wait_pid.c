#include "../wait.h"
#include "../uint64.h"
#include "../windoze.h"

#if WINDOWS_NATIVE
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#endif

int
wait_pid(int pid, int* wstat) {
#if WINDOWS_NATIVE
  DWORD exitcode = 0;
  HANDLE hproc;
  int i;
  int ret;

  hproc = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION, FALSE, pid);

  for(;;) {

    ret = WaitForSingleObject(hproc, INFINITE);

    if(ret == WAIT_TIMEOUT)
      continue;
    if(ret == WAIT_FAILED)
      return -1;

    if(ret == WAIT_OBJECT_0) {
      GetExitCodeProcess(hproc, &exitcode);
      CloseHandle(hproc);
      if(exitcode == STILL_ACTIVE)
        return -1;

      *wstat = exitcode;
      return 1;
    }
  }
  return -1;

#else
  return waitpid_nointr(pid, wstat, 0);
#endif
}
