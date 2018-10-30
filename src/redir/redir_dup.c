#include "scan.h"
#include "fd.h"
#include "fdtable.h"
#include "redir.h"
#include "expand.h"

/* do a dup-redirection
 * ----------------------------------------------------------------------- */
int redir_dup(struct nredir *nredir, stralloc *sa) {
  int ret;
  
  /* [n]>&- means closing a file descriptor */
  if(sa->len != 1 || sa->s[0] != '-') {
    int fd;
    
    scan_uint(sa->s, (unsigned int *)&fd);
    
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

