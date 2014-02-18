#include "fdtable.h"
#include "fdstack.h"
#include "fd.h"

/* handle unexpected fds
 * 
 * e = expected fd
 * u = unexpected fd
 * 
 * when flags is FDTABLE_FORCE then the fd at the expected position
 * is not added
 * 
 * ----------------------------------------------------------------------- */
void fdtable_unexpected(int e, int u, int flags)
{
  int mode;
  struct fd *fd;
  
  if(flags & FDTABLE_FORCE)
    e++;
  
  while(e < u)
  {
    /* if there is not already a virtual fd 
        for this effective fd then add one */
    if(!fd_list[e])
    {
      mode = fdtable_check(fd_exp);
      fd = fdtable_newfd(e, &fdstack_root, mode);
      fd_setfd(fd, e);
    }
    
    e++;
  }
}

  
