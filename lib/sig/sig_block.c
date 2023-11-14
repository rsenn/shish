#include "../windoze.h"
#include "../sig.h"

#include <signal.h>

void
sig_block(int signum) {
#if !WINDOWS_NATIVE
  sigset_type ss;
  sig_emptyset(&ss);
  sigprocmask(SIG_SETMASK, 0, &ss);

  sig_addset(&ss, signum);
  sigprocmask(SIG_BLOCK, &ss, 0);
#endif
}
