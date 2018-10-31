#include <termios.h>
#include "term.h"

struct termios term_tcattr;

/* backup and set terminal attributes
 * ----------------------------------------------------------------------- */
int term_attr(int fd, int set) {
  int ret;

  
  
  if((ret = tcgetattr(fd, &term_tcattr)) == 0 && set) {
    struct termios newattr;
    /* backup tty settings */
    newattr = term_tcattr;
    /* set non-canonical and disable echo */
    newattr.c_lflag &= ~(ICANON|ECHO|ISIG);
    return tcsetattr(fd, TCSANOW, &newattr);
  }
  return ret;
}

