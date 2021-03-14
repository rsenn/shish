#include "../term.h"
#include "../../lib/windoze.h"
#if !WINDOWS_NATIVE && !defined(__MINGW64__)
#include <termios.h>

struct termios term_tcattr;
#endif

/* backup and set terminal attributes
 * ----------------------------------------------------------------------- */
int
term_attr(int fd, int set, struct termios* oldattr) {
  int ret;

#include "../../lib/windoze.h"
#if !WINDOWS_NATIVE && !defined(__MINGW64__)
  if((ret = tcgetattr(fd, oldattr)) == 0 && set) {
    struct termios newattr;
    /* backup tty settings */
    newattr = *oldattr;
    /* set non-canonical and disable echo */
    newattr.c_lflag &= ~(ICANON | ECHO | ISIG);
    return tcsetattr(fd, TCSANOW, &newattr);
  }
#endif

  return ret;
}
