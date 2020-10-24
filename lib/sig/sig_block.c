#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <signal.h>
#include "../sig.h"

void
sig_block(int sig) {
#ifdef HAVE_SIGPROCMASK
  sigset_t ss;
  sigemptyset(&ss);
  sigaddset(&ss, sig);
  sigprocmask(SIG_BLOCK, &ss, (sigset_t*)0);
#else
  sigblock(1 << (sig - 1));
#endif
}
