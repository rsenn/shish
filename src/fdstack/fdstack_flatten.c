#include "../fd.h"
#include "../fdtable.h"
#include "../fdstack.h"

/* closes all fds which are not on top
 * ----------------------------------------------------------------------- */
void
fdstack_flatten(void) {
  struct fdstack* st;
  struct fd *fd, *next;

  for(st = fdstack; st; st = st->parent)
    for(fd = st->list; fd; fd = next) {
      next = fd->next;

      if(fd != fdtable[fd->n])
        fd_pop(fd);
    }
}
