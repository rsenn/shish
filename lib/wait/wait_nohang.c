#include "../wait.h"
#include "../windoze.h"

#if !WINDOWS_NATIVE
#include <sys/types.h>
#include <sys/wait.h>
#endif

int
wait_nohang(int* wstat) {
#if WINDOWS_NATIVE
//#warning No windows implementation
#elif defined(__unix__)
  return waitpid(-1, wstat, WNOHANG);
#else
  return wait3(wstat, WNOHANG, (struct rusage*)0);
#endif
}
