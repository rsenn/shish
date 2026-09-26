#include "../sh.h"
#include "../../lib/sig.h"
#include "../../lib/windoze.h"
#include "builtin_config.h"

#if !WINDOWS_NATIVE
#include <signal.h>

#if BUILTIN_TRAP
int trap_ignores(int sig);
#endif

static const int sh_sigignore_list[] = {SIGINT, SIGQUIT, SIGTERM};

static void
sh_sigset(int sig, void (*handler)(int)) {
  struct sigaction sa;

  sa.sa_handler = handler;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sig_action(sig, &sa, NULL);
}
#endif

/* an interactive shell ignores INT, QUIT and TERM for itself, unless
 * they were already ignored on entry (then they stay ignored)
 * ----------------------------------------------------------------------- */
void
sh_sigignore(void) {
#if !WINDOWS_NATIVE
  unsigned i;

  if(!sh_interactive)
    return;

  for(i = 0; i < sizeof(sh_sigignore_list) / sizeof(sh_sigignore_list[0]); i++) {
    struct sigaction cur;

    /* a trap's handler or an ignore already in place is left alone */
    if(sig_was_ignored(sh_sigignore_list[i]) ||
       (sigaction(sh_sigignore_list[i], NULL, &cur) == 0 && cur.sa_handler != SIG_DFL))
      continue;

    sh_sigset(sh_sigignore_list[i], SIG_IGN);
  }
#endif
}

/* in a forked child, the ignore above must not be inherited: reset to
 * default unless the signal was ignored on entry or by "trap '' SIG"
 * ----------------------------------------------------------------------- */
void
sh_sigrestore(void) {
#if !WINDOWS_NATIVE
  unsigned i;

  if(!sh_interactive)
    return;

  for(i = 0; i < sizeof(sh_sigignore_list) / sizeof(sh_sigignore_list[0]); i++) {
    int sig = sh_sigignore_list[i];

    if(sig_was_ignored(sig))
      continue;
#if BUILTIN_TRAP
    if(trap_ignores(sig))
      continue;
#endif
    sh_sigset(sig, SIG_DFL);
  }
#endif
}
