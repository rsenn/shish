#include "../fdstack.h"

/* links an fd to the specfied stack level
 * ----------------------------------------------------------------------- */
void
fdstack_link(struct fdstack* st, struct fd* fd) {

  struct fd** ptr;

  for(ptr = &st->list; *ptr; ptr = &(*ptr)->next)
    ;

  fd->link = ptr;
  fd->next = 0;

  *ptr = fd;

  fd->stack = st;
}
