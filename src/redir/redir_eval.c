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

  /* do the appropriate redirection -- redir_here() hands sa->s off to
     fd_here() (which reads straight from that buffer, freeing it only
     when the fd itself is later closed), so sa must not be touched
     again after that call; every other action is done with it once
     the switch returns, so free it here rather than relying on each
     callee to free its own copy (redir_open() never did, leaking sa->s
     on every plain "> file"/"< file" redirection -- confirmed leak,
     see fixes/133's regression test in tests/fixed.sh). */
  switch(nredir->flag & R_ACT) {
    case R_OPEN: r = redir_open(nredir, &sa); stralloc_free(&sa); break;
    case R_HERE: r = redir_here(nredir, &sa); break;
    default: r = redir_dup(nredir, &sa); stralloc_free(&sa); break;
  }

  /*  if(nredir->flag & R_NOW)
      return fdtable_resolve(nredir->d, FDTABLE_MOVE);*/

  return r;
}
