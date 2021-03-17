#include "../sig.h"
#include "../windoze.h"

#include <signal.h>

void
sig_pause(void) {
  sigset_t ss;
  sigemptyset(&ss);
#if !WINDOWS_NATIVE
  sigsuspend(&ss);
#endif
}
