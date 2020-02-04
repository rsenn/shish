#include "../term.h"
#include "../../lib/windoze.h"
#if !WINDOWS_NATIVE && !defined(__MINGW64__)
#include <sys/ioctl.h>
#include <termios.h>

struct winsize term_size;
#endif

/* get window size on a resize interrupt
 * ----------------------------------------------------------------------- */
void
term_winsize(void) {
#ifdef TIOCGWINSZ
  struct winsize sz;

  if(ioctl(term_input.fd, TIOCGWINSZ, &sz) == 0)
    term_size = sz;
#endif
}
