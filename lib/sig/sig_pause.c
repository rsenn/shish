#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <signal.h>
#include "../sig.h"

void
sig_pause(void) {
#ifdef HAVE_SIGPROCMASK
  sigset_t ss;
  sigemptyset(&ss);
  sigsuspend(&ss);
#elif defined(HAVE_SIGPAUSE)
  sigpause(0);
#endif
}
