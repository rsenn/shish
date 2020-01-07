#include "fdtable.h"
#include "sh.h"

/* create an (fd) entry on the current level
 * ----------------------------------------------------------------------- */
struct fd*
fd_new(int fd, int mode) {
  return fdtable_newfd(fd, sh->fdstack, mode);
}
