#include "../windoze.h"
#include "../sig.h"

#include <signal.h>

void
sig_blocknone(void) {
#if !WINDOWS_NATIVE
  sigset_t ss;

  sigemptyset(&ss);
  sigprocmask(SIG_SETMASK, &ss, 0);
#endif
}
