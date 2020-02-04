#include "../fdstack.h"
#include <assert.h>

/* discards current io context and gets the parent
 * ----------------------------------------------------------------------- */
void
fdstack_pop(struct fdstack* st) {
  struct fd* fd;
  struct fd* next;

  assert(fdstack == st);

  /* close all files and free filenames opened on this level */
  for(fd = st->list; fd; fd = next) {
    next = fd->next;
    fd_pop(fd);
  }

  /* now leave this level */
  fdstack = st->parent;
}
