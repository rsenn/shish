#include "../windoze.h"
#include "../sig.h"

#include <signal.h>

void
sig_block(int signum) {
#if !WINDOWS_NATIVE
  sigset_t ss;
  sigemptyset(&ss);
  sigprocmask(SIG_SETMASK, 0, &ss);

  sigaddset(&ss, signum);
  sigprocmask(SIG_BLOCK, &ss, 0);
#endif
}
