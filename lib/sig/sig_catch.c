#include "../sig.h"

/* MT-unsafe */

int
sig_catch(int sig, sighandler_t_ref f) {
#if !(defined(_WIN32) || defined(__MSYS__))
  struct sigaction ssa = {
      .sa_handler = f,
      .sa_mask = 0,
      .sa_flags = SA_MASKALL | SA_NOCLDSTOP,
      .sa_restorer = 0,
  };

  return sig_catcha(sig, &ssa);
#endif
}
