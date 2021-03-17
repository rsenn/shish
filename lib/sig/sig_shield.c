#include "../windoze.h"
#include "../sig.h"

#include <signal.h>

void
sig_shield(void) {
#if !WINDOWS_NATIVE
  sigset_t ss;
  sigemptyset(&ss);
  sigaddset(&ss, SIGTERM);
  sigaddset(&ss, SIGQUIT);
  sigaddset(&ss, SIGABRT);
  sigaddset(&ss, SIGINT);
  sigaddset(&ss, SIGPIPE);
  sigaddset(&ss, SIGHUP);
  sigprocmask(SIG_BLOCK, &ss, 0);
#endif
}
