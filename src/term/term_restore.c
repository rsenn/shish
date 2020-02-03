#include "term.h"
#include <signal.h>
#include "windoze.h"
#if !WINDOWS_NATIVE && !defined(__MINGW64__)
#include <termios.h>
#endif

/* restore old terminal attrs
 * ----------------------------------------------------------------------- */
void
term_restore(buffer* input) {
#ifdef TCSANOW
  tcsetattr(input->fd, TCSANOW, &term_tcattr);
#endif
#ifdef SIGWINCH
  signal(SIGWINCH, SIG_DFL);
#endif
}
