#include "../fd.h"
#include "../fdstack.h"
#include "../fdtable.h"

/* find next lowest file descriptor
 *
 * (assumed to be higher than the current one)
 * ----------------------------------------------------------------------- */
void
fdtable_up(void) {
  struct fd* fd;
  struct fdstack* st;

  if(fd_top < fd_expected)
    fd_top = fd_expected;

  /* find next lowest */
again:
  fd_expected++;

  for(st = fdstack; st; st = st->parent) {
    for(fd = st->list; fd; fd = fd->next) {
      if(fd->e == fd_expected)
        goto again;
    }
  }
}
