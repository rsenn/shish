#include "../expand.h"
#include "../trace.h"
#include "../redir.h"

/* set up an fd for a here-document
 * ----------------------------------------------------------------------- */
int
redir_here(struct nredir* nredir, stralloc* sa) {
  /* make the input buffer read from the stralloc */
  TRACE(TRACE_REDIR, "here", trace_int("fd", nredir->fdes), trace_int("len", sa->len));

  fd_here(nredir->fd, sa);

  return 0;
}
