#include "../sig.h"

#include <signal.h>

#ifdef SA_RESTART
int
sig_action(int sig, struct sigaction const* new, struct sigaction* old) {
  struct sigaction sanew, saold;
  if(((new->sa_flags& SA_MASKALL) ? sigfillset(&sanew.sa_mask)
                                  : sigemptyset(&sanew.sa_mask)) == -1)
    return -1;
  sanew.sa_handler = new->sa_handler;
  sanew.sa_flags = (new->sa_flags& SA_NOCLDSTOP) ? SA_NOCLDSTOP : 0;
#ifndef FLAG_PREFERSELECT
  sanew.sa_flags |= SA_RESTART;
#endif
  if(sigaction(sig, &sanew, &saold) < 0)
    return -1;
  if(old) {
    int r = sigismember(&saold.sa_mask, (sig == SIGTERM) ? SIGPIPE : SIGTERM);
    if(r < 0)
      return -1;
    old->sa_flags = 0;
    if(r)
      old->sa_flags |= SA_MASKALL;
    if(saold.sa_flags & SA_NOCLDSTOP)
      old->sa_flags |= SA_NOCLDSTOP;
    old->sa_handler = saold.sa_handler;
  }
  return 0;
}
#endif
