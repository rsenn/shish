#include "../wait.h"
#include "../windoze.h"

#if WINDOWS_NATIVE
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#endif

int
wait_pids_nohang(int const* pids, unsigned int len, int* wstat) {
#if WINDOWS_NATIVE
  DWORD exitcode = 0;
  HANDLE* handles = LocalAlloc(LHND, sizeof(HANDLE) * len);
  unsigned int i;
  int ret;

  for(i = 0; i < len; i++) {
    handles[i] =
        OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION, FALSE, pids[i]);
  }

  for(;;) {

    ret = WaitForMultipleObjects(len, handles, FALSE, 0);

    if(ret == WAIT_TIMEOUT)
      return 0;
    if(ret == WAIT_FAILED)
      return -1;

    for(i = 0; i < len; i++) {
      if(ret == WAIT_OBJECT_0 + i) {
        GetExitCodeProcess(handles[i], &exitcode);
        CloseHandle(handles[i]);
        if(exitcode == STILL_ACTIVE)
          i = -2;
        break;
      }
    }
    if(i < len) {
      *wstat = exitcode;
      return 1 + i;
    }
  }
  return -1;
#else
  for(;;) {
    int w;
    int r = wait_nohang(&w);
    if(!r || (r == (int)-1))
      return (int)r;
    {
      unsigned int i = 0;
      for(; i < len; i++)
        if(r == pids[i])
          break;
      if(i < len) {
        *wstat = w;
        return 1 + i;
      }
    }
  }
  return -1;
#endif
}
