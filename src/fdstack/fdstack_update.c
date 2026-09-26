#include "../trace.h"
#include "../fd.h"
#include "../fdstack.h"

/* update efd of all duplicates of fd
 * ----------------------------------------------------------------------- */
void
fdstack_update(struct fd* dup) {
  struct fdstack* st;
  struct fd* fd;

  TRACE(TRACE_FDSTACK, "update", trace_int("n", dup->n), trace_int("e", dup->e));

  for(st = fdstack; st; st = st->parent)
    for(fd = st->list; fd; fd = fd->next) {
      if(fd->dup == dup)
        fd->e = dup->e;
    }
}
