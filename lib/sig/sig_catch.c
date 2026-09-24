#include "../sig.h"

/* MT-unsafe */

int
sig_catch(int sig, sighandler_t_ref f) {
  struct sigaction ssa = {0};

  ssa.sa_handler = f;
  ssa.sa_flags = SA_MASKALL | SA_NOCLDSTOP;

  return sig_catcha(sig, &ssa);
}
