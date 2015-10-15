#include "fdstack.h"

/* unlinks an fd from the stack
 * ----------------------------------------------------------------------- */
void fdstack_unlink(struct fd *fd) {
  if(fd->next)
    fd->next->link = fd->link;

  *fd->link = fd->next;
  fd->stack = 0;
}
