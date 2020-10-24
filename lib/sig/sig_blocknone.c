#include <signal.h>
#include "../sig.h"

void
sig_blocknone(void) {
#ifdef HAVE_SIGPROCMASK
  sigset_t ss;
  sigemptyset(&ss);
  sigprocmask(SIG_SETMASK, &ss, (sigset_t*)0);
#else
  sigsetmask(0);
#endif
}
