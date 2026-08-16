#include "../fd.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../../lib/windoze.h"

#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* prepare I/O stack for execve()
 *
 * all pending dup()s will be done and fds with vfd != efd will be
 * mapped to their effective file descriptor (normalizing), so vfd will
 * be equal to efd and the fd entry could be removed (but it usually
 * won't because execve() discards mapped memory).
 *
 * WARNING: you should have fork()ed before using this function!!
 * ----------------------------------------------------------------------- */
int
fdtable_exec(void) {
  int i;

  /* the sources can be closed if an execve() follows */
  while(fdtable[STDSRC_FILENO])
    fd_pop(fdtable[STDSRC_FILENO]);

  /* open every still-pending real file first, before resolving any
   * dups. A "N<&M" dup can still have its source unopened at this
   * point (e.g. "cmd 9<in0 8<&9 0<&3") -- resolving 0 before 9 got a
   * chance to open would just fail against the source's unresolved -1.
   * Opening all real files up front means every dup below finds its
   * source already correct. */
  fdtable_foreach(i) {
    if((fdtable[i]->mode & FD_OPEN) && fdtable_open(fdtable[i], FDTABLE_MOVE) == FDTABLE_ERROR)
      return -1;
  }

  fdtable_foreach(i) {
    if(fdtable_resolve(fdtable[i], FDTABLE_FORCE) == FDTABLE_ERROR)
      return -1;
  }

  return 0;
}
