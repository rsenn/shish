#include "../../lib/windoze.h"
#include "../term.h"
#include <signal.h>

#if !WINDOWS_NATIVE
#include <termios.h>
#endif

/* restore old terminal attrs
 * ----------------------------------------------------------------------- */
void
term_restore(int fd, const struct termios* tcattr) {
#ifdef TCSANOW
  tcsetattr(fd, TCSANOW, (struct termios*)tcattr);
#endif
#ifdef SIGWINCH
  signal(SIGWINCH, SIG_DFL);
#endif
}
