#include "../windoze.h"
#include "../sig.h"

/* MT-unsafe */

int
sig_push(int sig, sighandler_t_ref f) {
#if !WINDOWS_NATIVE
  struct sigaction ssa = {0};

  /* sig_action() (called via sig_pusha() below) never reads ssa.sa_mask
     as an actual signal set -- it only looks at the SA_MASKALL/
     SA_NOCLDSTOP bits in ssa.sa_flags and builds its own real sigset_t
     from those. The two flags therefore belong in sa_flags, not
     sa_mask; writing them into sa_mask through a cast to unsigned
     long* (sig_push's previous body) is undefined behavior on any ABI
     where sigset_t isn't itself a plain unsigned long (glibc's is a
     128-byte struct) and, regardless of ABI, leaves the real
     sa_flags field uninitialized (sig-push-sigset-cast, fixes/76). */
  ssa.sa_handler = f;
  ssa.sa_flags = SA_MASKALL | SA_NOCLDSTOP;

  return sig_pusha(sig, &ssa);
#endif
}
