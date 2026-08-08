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
     defined POSIX no-op: dup2(fd, fd) succeeds trivially, without even
     requiring fd to already be open, whenever both arguments are equal.
     This is a common, harmless idiom in real scripts -- a bare ">&1"
     (fdes defaults to 1, target word is "1") inside a "{ ...; } > file"
     group, exactly what autoconf's own config.status emits -- so
     rejecting it broke configure scripts outright
     (self-referring-duplicate-rejected-config-status, BUGS).

     fd_new()/fd_push() below are about to overwrite fdtable[nredir->fdes]
     -- for a persistent "exec" redirection reusing the same fd number
     this is destructive (fd_reinit() closes whatever real resource the
     slot owned directly), so the snapshot has to happen before that
     call, and only stays usable afterward if it's itself a dup of some
     *other*, distinct fd number's entry (chased via ->dup): that other
     entry is untouched by this fd's own reinitialization, whereas a
     directly-owned resource is not recoverable once destroyed. */
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

  /* do the appropriate redirection -- redir_here() hands sa->s off to
     fd_here() (which reads straight from that buffer, freeing it only
     when the fd itself is later closed), so sa must not be touched
     again after that call; every other action is done with it once
     the switch returns, so free it here rather than relying on each
     callee to free its own copy (redir_open() never did, leaking sa->s
     on every plain "> file"/"< file" redirection -- confirmed leak,
     see fixes/133's regression test in tests/fixed.sh). */
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
