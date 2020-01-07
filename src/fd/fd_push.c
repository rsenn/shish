#include "fd.h"
#include "fdstack.h"
#include "fdtable.h"

/* push an (fd) to the top fdtable
 * ----------------------------------------------------------------------- */
struct fd*
fd_push(struct fd* fd, int n, int mode) {
  fd_init(fd, n, mode);
  fdtable_pos = &fdtable[n];
  fdtable_link(fd);
  fdstack_link(fdstack, fd);
  return fd;
}
