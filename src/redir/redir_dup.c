#include "../expand.h"
#include "../fd.h"
#include "../fdtable.h"
#include "../redir.h"
#include "../../lib/scan.h"

/* do a dup-redirection
 *
 * caller (redir_eval.c) owns sa and frees it once this returns
 * ----------------------------------------------------------------------- */
int
redir_dup(struct nredir* nredir, stralloc* sa) {
  int ret;

  /* [n]>&- means closing a file descriptor */
  if(sa->len != 1 || sa->s[0] != '-') {
    int fd = 0;

    scan_uint(sa->s, (unsigned int*)&fd);

    /* a bogus fd number (e.g. "3<&99999") must not reach fd_dup()'s
       fdtable[fd] lookup unchecked */
    if(fd < 0 || fd >= FD_MAX) {
      fd_error(fd, "bad file descriptor");
      return 1;
    }

    /* dup only if the filedescriptors are different -- redir_eval()
       already special-cases every self-referencing duplicate it can
       safely turn into the POSIX-defined no-op; reaching this point
       with fdes == fd means it couldn't (the fd directly owned a real
       resource that a persistent "exec" redirection just destroyed
       in-place reusing the same slot), so there is nothing left to
       duplicate from and this has to stay an error. */
    if(nredir->fdes == fd) {
      fd_error(fd, "self-referring duplicate");
      return 1;
    }

    ret = fd_dup(nredir->fd, fd);
  } else
    ret = fd_null(nredir->fd);

  /*  if(nredir->flag & R_NOW)
      fdtable_dup(nredir->fd, FDTABLE_MOVE);*/

  return ret;
}
