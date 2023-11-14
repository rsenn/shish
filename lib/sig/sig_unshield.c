#include "../windoze.h"
#include "../sig.h"

#include <signal.h>

void
sig_unshield(void) {
#if !WINDOWS_NATIVE
  sigset_type ss;

  sig_emptyset(&ss);

  sig_addset(&ss, SIGTERM);
  sig_addset(&ss, SIGQUIT);
  sig_addset(&ss, SIGABRT);
  sig_addset(&ss, SIGINT);
  sig_addset(&ss, SIGPIPE);
  sig_addset(&ss, SIGHUP);

  sigprocmask(SIG_UNBLOCK, &ss, 0);
#endif
}
