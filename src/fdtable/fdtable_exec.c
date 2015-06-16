#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "sh.h"
#include "fd.h"
#include "fdtable.h"

/* prepare I/O stack for execve()
 * 
 * all pending dup()s will be done and fds with vfd != efd will be 
 * mapped to their effective file descriptor (normalizing), so vfd will
 * be equal to efd and the fd entry could be removed (but it usually 
 * won't because execve() discards mapped memory).
 * 
 * WARNING: you should have fork()ed before using this function!!
 * ----------------------------------------------------------------------- */
int fdtable_exec(void)
{
  int i;

  /* the sources can be closed if an execve() follows */
  while(fdtable[STDSRC_FILENO])
    fd_pop(fdtable[STDSRC_FILENO]);
  
  fdtable_foreach(i)
    if(fdtable_resolve(fdtable[i], FDTABLE_FORCE) == FDTABLE_ERROR)
      return -1;
  
  return 0;
}

