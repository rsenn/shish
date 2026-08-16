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

  /* "[n]<&n"/"[n]>&n" (source and target the same descriptor) is a
   * defined POSIX no-op: dup2(fd, fd) succeeds trivially whenever both
   * arguments are equal, without requiring fd to already be open. A
   * common idiom -- a bare ">&1" inside "{ ...; } > file", which
   * autoconf's config.status emits.
   *
   * fd_new()/fd_push() below are about to overwrite
   * fdtable[nredir->fdes], which is destructive for a persistent
   * "exec" redirection reusing the same fd number. So the snapshot
   * must happen before that call, and only stays usable afterward if
   * it's a dup of some *other*, distinct fd number's entry (chased via
   * ->dup) -- a directly-owned resource is not recoverable once
   * destroyed. */
  {
    struct fd* selfdup_src = NULL;
    int selfdup = 0;

    if((nredir->flag & R_DUP) && !(nredir->flag & R_OPEN) && (sa.len != 1 || sa.s[0] != '-')) {
      int selffd = 0;

      scan_uint(sa.s, (unsigned int*)&selffd);

      if(selffd == nredir->fdes) {
        selfdup = 1;
        selfdup_src = fdtable_ok(selffd) ? fdtable[selffd] : NULL;

        while(selfdup_src && selfdup_src->dup)
          selfdup_src = selfdup_src->dup;
      }
    }

    /* setup up a new d for the redirection */
    nredir->fd = !d ? fd_new(nredir->fdes, mode) : fd_push(d, nredir->fdes, mode);

    if(selfdup && selfdup_src != nredir->fd) {
      if(selfdup_src) {
        nredir->fd->r = selfdup_src->r;
        nredir->fd->w = selfdup_src->w;
        nredir->fd->name = selfdup_src->name;
        nredir->fd->dup = selfdup_src;
        nredir->fd->e = selfdup_src->e;
        nredir->fd->mode |= (selfdup_src->mode & FD_TYPE) | FD_DUP;
        nredir->fd->dev = selfdup_src->dev;
      }

      stralloc_free(&sa);
      return 0;
    }
  }

  /* do the appropriate redirection. redir_here() hands sa->s off to
   * fd_here() (which reads straight from that buffer, freeing it only
   * when the fd is later closed), so sa must not be touched again
   * after that call. Every other action is done with sa once the
   * switch returns, so free it here rather than relying on each
   * callee to free its own copy. */
  switch(nredir->flag & R_ACT) {
    case R_OPEN:
      r = redir_open(nredir, &sa);
      stralloc_free(&sa);
      break;
    case R_HERE: r = redir_here(nredir, &sa); break;
    default:
      r = redir_dup(nredir, &sa);
      stralloc_free(&sa);
      break;
  }

  /*  if(nredir->flag & R_NOW)
      return fdtable_resolve(nredir->d, FDTABLE_MOVE);*/

  return r;
}
