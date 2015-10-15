#include <termios.h>
#include <sys/ioctl.h>
#include "term.h"

struct winsize term_size;

/* get window size on a resize interrupt
 * ----------------------------------------------------------------------- */
void term_winsize(void) {
  struct winsize sz;

  if(ioctl(term_input.fd, TIOCGWINSZ, &sz) == 0)
    term_size = sz;
}

