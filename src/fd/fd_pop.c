#include "fd.h"
#include "fdstack.h"
#include "fdtable.h"

/* close fd, unlink from the stack and free its ressources
 * ----------------------------------------------------------------------- */
void fd_pop(struct fd *fd) {
  fd_close(fd);
  fdtable_unlink(fd);
  fdstack_unlink(fd);
  fd_free(fd);

}
