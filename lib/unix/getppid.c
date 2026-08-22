#include "../windoze.h"

#if WINDOWS_NATIVE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <sys/types.h>

/* mingw has no getppid(): find our own entry in a process snapshot
 * and read its parent PID back out of it */
pid_t
getppid(void) {
  HANDLE snap;
  PROCESSENTRY32 pe;
  DWORD pid = GetCurrentProcessId();
  pid_t ppid = 0;

  snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

  if(snap == INVALID_HANDLE_VALUE)
    return 0;

  pe.dwSize = sizeof(pe);

  if(Process32First(snap, &pe)) {
    do {
      if(pe.th32ProcessID == pid) {
        ppid = (pid_t)pe.th32ParentProcessID;
        break;
      }
    } while(Process32Next(snap, &pe));
  }

  CloseHandle(snap);
  return ppid;
}
#endif /* WINDOWS_NATIVE */
