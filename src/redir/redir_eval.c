#include "../expand.h"
#include "../fd.h"
#include "../fdtable.h"
#include "../redir.h"
#include "../../lib/scan.h"
#include "../tree.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* evaluate a redirection
 *
 * fd is assumed to be a freshly allocated structure or NULL, if its
 * NULL a persistent redirection is assumed (whose fd will be malloced)
 * ----------------------------------------------------------------------- */
int
redir_eval(struct nredir* nredir, struct fd* d, int rfl) {
  int mode, r;
  stralloc sa;

  stralloc_init(&sa);
  expand_copysa(nredir->word, &sa, 0);
  stralloc_nul(&sa);

  /* set the initial d mode */
  mode = nredir->flag & (R_IN | R_OUT);

  /* additional redirection mode */
  nredir->flag |= rfl;

  /* setup up a new d for the redirection */
  nredir->fd = !d ? fd_new(nredir->fdes, mode) : fd_push(d, nredir->fdes, mode);

  /* do the appropriate redirection */
  switch(nredir->flag & R_ACT) {
    case R_OPEN: r = redir_open(nredir, &sa); break;
    case R_HERE: r = redir_here(nredir, &sa); break;
    default: r = redir_dup(nredir, &sa); break;
  }

  /*  if(nredir->flag & R_NOW)
      return fdtable_resolve(nredir->d, FDTABLE_MOVE);*/

  return r;
}
