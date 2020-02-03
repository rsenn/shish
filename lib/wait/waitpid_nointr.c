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
#else
  int r;
  do {
    r = waitpid(pid, wstat, flags);
  } while((r == (int)-1) && (errno == EINTR));
  return r;
#endif
}
