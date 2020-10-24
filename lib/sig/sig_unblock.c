#include <signal.h>
#include "../sig.h"

void
sig_unblock(int sig) {
#ifdef HAVE_SIGPROCMASK
  sigset_t ss;
  sigemptyset(&ss);
  sigaddset(&ss, sig);
  sigprocmask(SIG_UNBLOCK, &ss, (sigset_t*)0);
#else
  sigsetmask(sigsetmask(~0) & ~(1 << (sig - 1)));
#endif
}
