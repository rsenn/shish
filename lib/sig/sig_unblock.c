#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <signal.h>
#include "../sig.h"

void
sig_unblock(int sig) {
#if defined(HAVE_SIGPROCMASK)
  sigset_t ss;
  sigemptyset(&ss);
  sigaddset(&ss, sig);
  sigprocmask(SIG_UNBLOCK, &ss, (sigset_t*)0);
#elif defined(HAVE_SIGSETMASK)
  sigsetmask(sigsetmask(~0) & ~(1 << (sig - 1)));
#endif
}
