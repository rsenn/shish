#include "../expand.h"
#include "../fd.h"
#include "../fdtable.h"
#include "../redir.h"
#include "../tree.h"

/* open a file for redirection
 * ----------------------------------------------------------------------- */
int
redir_open(struct nredir* nredir, stralloc* sa) {
  int mode = FD_FREENAME;

  /* prepare flags */
  if(nredir->flag & R_OUT) {
    /* check for appending mode */
    if(nredir->flag & R_APPEND)
      mode |= FD_APPEND;
  }

  /* MISSING: no-clobbering (with O_EXCL?) */
  fd_open(nredir->fd, shell_strdup(sa->s), mode);

  if(nredir->flag & R_NOW)
    return (fdtable_open(nredir->fd, FDTABLE_MOVE) == FDTABLE_ERROR);

  return 0;
}
