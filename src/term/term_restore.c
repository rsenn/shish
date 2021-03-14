#include "../term.h"
#include <signal.h>
#include "../../lib/windoze.h"
#if !WINDOWS_NATIVE && !defined(__MINGW64__)
#include <termios.h>
#endif

/* restore old terminal attrs
 * ----------------------------------------------------------------------- */
void
term_restore(int fd, const struct termios* tcattr) {
#ifdef TCSANOW
  tcsetattr(fd, TCSANOW, tcattr);
#endif
#ifdef SIGWINCH
  signal(SIGWINCH, SIG_DFL);
#endif
}
