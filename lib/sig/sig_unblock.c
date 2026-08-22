#include "../windoze.h"
#include "../sig.h"

#include <signal.h>

void
sig_unblock(int signum) {
#if !WINDOWS_NATIVE
  sigset_t ss;

  sigemptyset(&ss);
  sigaddset(&ss, signum);
  sigprocmask(SIG_UNBLOCK, &ss, 0);
#endif
}
