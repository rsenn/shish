#include "../sig.h"

#include <signal.h>

/* POSIX 2.11: a signal that is already being ignored when the shell
 * starts must stay ignored -- trap/reset requests for it are silently
 * rejected. Snapshotting once here, before sig_push()/sig_catch() ever
 * runs, lets trap_install()/trap_uninstall() (src/builtin/builtin_trap.c)
 * check "was this ignored on entry" without disturbing the disposition
 * itself: sigaction(sig, 0, &old) only reads, never installs.
 * ----------------------------------------------------------------------- */
#ifdef SHISH_NSIG
static unsigned char sig_ignored[SHISH_NSIG - 1];

void
sig_snapshot(void) {
#if !WINDOWS_NATIVE
  int sig;
  struct sigaction sa;

  for(sig = 1; sig < SHISH_NSIG; sig++) {
    if(sigaction(sig, 0, &sa) == 0 && sa.sa_handler == SIG_IGN)
      sig_ignored[sig - 1] = 1;
  }
#endif
}

int
sig_was_ignored(int sig) {
  if((sig <= 0) || (sig >= SHISH_NSIG))
    return 0;

  return sig_ignored[sig - 1];
}
#else
void
sig_snapshot(void) {
}

int
sig_was_ignored(int sig) {
  (void)sig;
  return 0;
}
#endif
