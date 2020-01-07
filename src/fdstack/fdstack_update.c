#include "fd.h"
#include "fdstack.h"

/* update efd of all duplicates of fd
 * ----------------------------------------------------------------------- */
void
fdstack_update(struct fd* dup) {
  struct fdstack* st;
  struct fd* fd;

  for(st = fdstack; st; st = st->parent)
    for(fd = st->list; fd; fd = fd->next) {
      if(fd->dup == dup)
        fd->e = dup->e;
    }
}
