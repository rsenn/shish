#include "redir.h"
#include "expand.h"

/* set up an fd for a here-document
 * ----------------------------------------------------------------------- */
int redir_here(struct nredir *nredir, stralloc *sa)
{
  /* make the input buffer read from the stralloc */
  fd_here(nredir->fd, sa);
  
  return 0;
}
