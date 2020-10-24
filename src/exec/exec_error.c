#include <errno.h>

/* map errors to posix
 *
 * ENOENT -> 127
 * EACCES -> 126
 *
 * other: -> 1
 * ----------------------------------------------------------------------- */
int
exec_error(void) {
  int ret;

  switch(errno) {
    case ENOENT: ret = 127; break;
    case EACCES: ret = 126; break;
    default: ret = 1; break;
  }

  errno = 0;
  return ret;
}
