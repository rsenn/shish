#include "../term.h"

#ifdef HAVE_WINSIZE
struct winsize term_size;
#endif

/* get window size on a resize interrupt
 * ----------------------------------------------------------------------- */
void
term_winsize(void) {
#ifdef HAVE_WINSIZE
  struct winsize sz;

  if(ioctl(term_input.fd, TIOCGWINSZ, &sz) == 0)
    term_size = sz;
#endif
}
