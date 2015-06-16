#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "scan.h"
#include "fd.h"
#include "fdtable.h"
#include "tree.h"
#include "redir.h"
#include "expand.h"

/* evaluate a redirection
 * 
 * fd is assumed to be a freshly allocated structure or NULL, if its 
 * NULL a persistent redirection is assumed (whose fd will be malloced)
 * ----------------------------------------------------------------------- */
int redir_eval(struct nredir *nredir, struct fd *fd, int rfl)
{
  int mode;
  int r;
  stralloc sa;
  
  stralloc_init(&sa);
  expand_copysa((union node *)nredir, &sa, X_NOSPLIT);
  stralloc_nul(&sa);
  
  /* set the initial fd mode */
  mode = nredir->flag & (R_IN|R_OUT);

  /* additional redirection mode */
  nredir->flag |= rfl;
  
  /* setup up a new fd for the redirection */
  nredir->fd = !fd ? fd_new(nredir->fdes, mode) :
                fd_push(fd, nredir->fdes, mode);
      
  /* do the appropriate redirection */
  switch(nredir->flag & R_ACT)
  {
    case R_OPEN: r = redir_open(nredir, &sa); break;
    case R_HERE: r = redir_here(nredir, &sa); break;
    default: r = redir_dup(nredir, &sa); break;
  }

/*  if(nredir->flag & R_NOW)
    return fdtable_resolve(nredir->fd, FDTABLE_MOVE);*/
  
  return r;
}


