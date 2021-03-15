#include "../exec.h"
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
    case ENOENT: ret = EXIT_NOTFOUND; break;
    case EACCES: ret = EXIT_NOEXEC; break;
    default: ret = 1; break;
  }

  errno = 0;
  return ret;
}
