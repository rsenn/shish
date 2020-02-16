#include "../fdstack.h"
#include "../fdtable.h"

/* create an fd entry on the specified stack level
 * ----------------------------------------------------------------------- */
struct fd*
fdtable_newfd(int n, struct fdstack* st, int mode) {
  struct fd* fd;

  /* search for a present fd on the fdstack */
  if((fd = fdstack_search(st, n))) {
    /* if found, take it over */
    return fd_reinit(fd, mode);
  }

  /* otherwise allocate and initialize the new fd entry */
  fd = fd_mallocb();

  fd_init(fd, n, D_FREE | mode);
  fd_setbuf(fd, &fd[1], FD_BUFSIZE);

  fdstack_link(st, fd);
  fdtable_link(fd);

  return fd;
}
