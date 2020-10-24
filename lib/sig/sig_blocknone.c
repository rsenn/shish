#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <signal.h>
#include "../sig.h"

void
sig_blocknone(void) {
#ifdef HAVE_SIGPROCMASK
  sigset_t ss;
  sigemptyset(&ss);
  sigprocmask(SIG_SETMASK, &ss, (sigset_t*)0);
#elif defined(HAVE_SIGSETMASK)
  sigsetmask(0);
#endif
}
