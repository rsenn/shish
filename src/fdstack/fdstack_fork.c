#include "../fdstack.h"
#include "../fdtable.h"

/* ----------------------------------------------------------------------- */
int
fdstack_fork(unsigned int n, struct fd* fds) {
  struct fdstack* st;

  for(st = fdstack; st; st = st->parent) {
    struct fd* fd;
    for(fd = st->list; fd; fd = fd->next) {
      /* make files out of stralloc here-docs */
      if((fd->mode & FD_HERE) == FD_HERE) {
        if(fdtable_here(fd, FDTABLE_MOVE) == FDTABLE_ERROR)
          return -1;
      }
    }
  }

  return 0;
}
