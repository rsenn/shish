#ifdef DEBUG
#include "fdstack.h"
#include "fd.h"

void fdstack_dump(void)
{
  struct fdstack *st;
  struct fd *fd;

  buffer_puts(fd_out->w, "  fd  name        level read-buffer                          write-buffer                        flags\n");
  buffer_puts(fd_out->w, "------------------------------------------------------------------------------------------------------------\n");

  for(st = fdstack; st; st = st->parent)
    for(fd = st->list; fd; fd = fd->next)
      fd_dump(fd);
}
#endif /* DEBUG */
