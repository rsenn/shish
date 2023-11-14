#include "../sig.h"
#include "../windoze.h"

#include <signal.h>

void
sig_pause(void) {
  sigset_type ss;
  sig_emptyset(&ss);
#if !WINDOWS_NATIVE
  sigsuspend(&ss);
#endif
}
