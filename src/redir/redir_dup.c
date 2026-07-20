#include "../expand.h"
#include "../fd.h"
#include "../fdtable.h"
#include "../redir.h"
#include "../../lib/scan.h"

/* do a dup-redirection
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
      stralloc_free(sa);
      return 1;
    }

    /* dup only if the filedescriptors are different */
    if(nredir->fdes == fd) {
      fd_error(fd, "self-referring duplicate");
      return 1;
    }

    ret = fd_dup(nredir->fd, fd);
  } else
    ret = fd_null(nredir->fd);

  /*  if(nredir->flag & R_NOW)
      fdtable_dup(nredir->fd, FDTABLE_MOVE);*/

  stralloc_free(sa);
  return ret;
}
