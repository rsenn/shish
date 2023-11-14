#include "../windoze.h"
#include "../sig.h"

#include <signal.h>

void
sig_unblock(int signum) {
#if !WINDOWS_NATIVE
  sigset_type ss;

  sig_emptyset(&ss);
  sigprocmask(SIG_SETMASK, 0, &ss);

  sig_delset(&ss, signum);
  sigprocmask(SIG_BLOCK, &ss, 0);
#endif
}
