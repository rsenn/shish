#include "term.h"
#include <signal.h>
#include <termios.h>

/* restore old terminal attrs
 * ----------------------------------------------------------------------- */
void
term_restore(buffer* input) {
  tcsetattr(input->fd, TCSANOW, &term_tcattr);
  signal(SIGWINCH, SIG_DFL);
}
