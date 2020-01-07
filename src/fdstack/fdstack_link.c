#include "fdstack.h"

/* links an fd to the specfied stack level
 * ----------------------------------------------------------------------- */
void
fdstack_link(struct fdstack* st, struct fd* fd) {
  fd->link = &st->list;
  fd->next = *fd->link;

  if(fd->next)
    fd->next->link = &fd->next;

  st->list = fd;
  fd->stack = st;
}
