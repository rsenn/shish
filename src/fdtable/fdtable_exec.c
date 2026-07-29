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

  /* open every still-pending real file first, before resolving
     anything else. A plain command's redirections are only ever
     *recorded* as parsed, with the real open()/dup2() deferred to
     here -- so a "N<&M" dup (FD_DUP mode) can easily still have its
     source (fd_dup() already flattens ->dup straight to it) sitting
     unopened (FD_OPEN mode) at this point, e.g. "cmd 9<in0 8<&9 ...
     0<&3". Resolving fd 0 before fd 9 ever got a chance to open would
     just fail outright (fdtable_dup() dup()'ing the still-unresolved
     -1 snapshotted at setup time) -- resolving every real open() up
     front first means every dup below finds its source already
     correct, however many of them share it and whatever fd number
     order they happen to occupy.
     (redir-fd-chain-resolves-to-invalid-fd, fixes/89) */
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
