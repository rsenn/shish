#include "fd.h"
#include "fdtable.h"
#include "fdstack.h"

/* find next lowest file descriptor
 * 
 * (assumed to be higher than the current one)
 * ----------------------------------------------------------------------- */
void fdtable_up(void)
{
  struct fd *fd;
  struct fdstack *st;
  
  if(fd_exp > fd_top)
    fd_top = fd_exp;
  
  /* find next lowest */
again:
  fd_exp++;
  
  for(st = fdstack; st; st = st->parent)
  {
    for(fd = st->list; fd; fd = fd->next)
    {
      if(fd->e == fd_exp)
        goto again;
    }
  }
}
