#include "../windoze.h"
#include "../sig.h"

#include <signal.h>

void
sig_blockset(const void* set) {
#if !WINDOWS_NATIVE
  sigprocmask(SIG_SETMASK, set, 0);
#endif
}
