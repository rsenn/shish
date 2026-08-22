#include "../fd.h"
#include "../fdtable.h"
#include "../redir.h"
#include "../tree.h"
#include "../sh.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* open the file of a persistent ("exec") "<file"/">file" redirection
 * before the fd entry it is going to replace is destroyed.
 *
 * the flags are worked out on a throwaway entry, exactly as
 * redir_open()/fd_open() would work them out on the real one.
 *
 *   struct nredir*  nredir  the redirection
 *   stralloc*       sa      the expanded file name (NUL terminated)
 *
 * returns the new kernel fd, or -1 with errno set
 * ----------------------------------------------------------------------- */
int
redir_preopen(struct nredir* nredir, stralloc* sa) {
  struct fd probe;
  int mode = 0, e;

  if(nredir->flag & R_OUT) {
    if(nredir->flag & R_APPEND)
      mode |= FD_APPEND;
    else if(sh->opts.noclobber)
      mode |= FD_EXCL;
  }

  fd_init(&probe, nredir->fdes, nredir->flag & (R_IN | R_OUT));
  fd_open(&probe, sa->s, mode);

  e = open(probe.name, probe.fl, (0666 & ~sh->umask));

  /* the fd number being replaced can be free at the kernel level
     while the shell still lists an entry for it, and open() then
     lands right on it -- where the fd_close() inside fd_new() would
     destroy what was just opened. Move it out of that slot. */
  if(e == nredir->fdes) {
    int moved = dup(e);

    if(fd_ok(moved)) {
      close(e);
      e = moved;
    }
  }

  return e;
}
