#include "../../lib/alloc.h"
#include "../expand.h"
#include "../trace.h"
#include "../fd.h"
#include "../fdtable.h"
#include "../redir.h"
#include "../tree.h"
#include "../sh.h"

/* open a file for redirection
 *
 *   struct nredir*  nredir   the redirection
 *   stralloc*       sa       the expanded file name (NUL terminated)
 *   int             preopen  an already open descriptor for that file
 *                            (redir_preopen()), or -1 to open it here
 * ----------------------------------------------------------------------- */
int
redir_open(struct nredir* nredir, stralloc* sa, int preopen) {
  int mode = FD_FREENAME;

  /* prepare flags */
  if(nredir->flag & R_OUT) {
    /* check for appending mode */
    if(nredir->flag & R_APPEND)
      mode |= FD_APPEND;
    else if(sh->opts.noclobber)
      mode |= FD_EXCL;
  }

  /* MISSING: no-clobbering (with O_EXCL?) */
  TRACE(TRACE_REDIR,
        "open",
        trace_int("fd", nredir->fdes),
        trace_str("path", sa->s),
        trace_raw("mode", (mode & FD_APPEND) ? "append" : (mode & FD_EXCL) ? "noclobber" : (nredir->flag & R_OUT) ? "trunc" : "read"),
        trace_int("preopen", preopen),
        trace_int("now", !!(nredir->flag & R_NOW)));

  fd_open(nredir->fd, str_dup(sa->s), mode);

  /* the file is already open (redir_preopen()): just hand the
     descriptor over instead of opening it a second time */
  if(fd_ok(preopen))
    return (fdtable_openfd(nredir->fd, preopen, FDTABLE_MOVE) == FDTABLE_ERROR);

  if(nredir->flag & R_NOW)
    return (fdtable_open(nredir->fd, FDTABLE_MOVE) == FDTABLE_ERROR);

  return 0;
}
