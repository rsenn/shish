#include "../windoze.h"
#include "../sig.h"

/* MT-unsafe */

int
sig_push(int sig, sighandler_t_ref f) {
#if !WINDOWS_NATIVE
  struct sigaction ssa;
  ssa.sa_handler = f;
  *((unsigned long*)&ssa.sa_mask) = SA_MASKALL | SA_NOCLDSTOP;
  return sig_pusha(sig, &ssa);
#endif
}
