#include "../expand.h"
#include "../trace.h"
#include "../fd.h"
#include "../fdtable.h"
#include "../redir.h"
#include "../../lib/byte.h"
#include "../../lib/scan.h"
#include "../sh.h"
#include "../tree.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* evaluate one redirection onto fdtable[nredir->fdes]
 *
 *   struct nredir*  nredir  the redirection
 *   struct fd*      d       fresh fd to fd_push(), or NULL for a persistent ("exec") one:
 *                           fdtable_newfd() then reuses the entry at that number, else mallocs
 *   int             rfl     extra R_* flags, or'ed into nredir->flag
 *
 * returns 0 on success, nonzero on failure
 * ----------------------------------------------------------------------- */
int
redir_eval(struct nredir* nredir, struct fd* d, int rfl) {
  int mode, r, preopen = -1;
  stralloc sa;

  stralloc_init(&sa);

  /* the operand gets tilde expansion (not a here-document body); the word
     is a bare N_ARGSTR chain, wrapped in an N_ARG for the tilde helpers and
     expanded on a private copy, since the parse tree is reused */
  {
    union node probe;

    byte_zero(&probe, sizeof(probe));
    probe.id = N_ARG;
    probe.narg.list = nredir->word;

    if(!(nredir->flag & R_HERE) && expand_tilde_needed(&probe)) {
      union node* wrap = tree_newnode(N_ARG);

      wrap->narg.list = tree_copy(nredir->word);
      expand_tilde_word(wrap);
      expand_copysa(wrap->narg.list, &sa, 0);
      tree_free(wrap);
    } else {
      expand_copysa(nredir->word, &sa, 0);
    }
  }

  stralloc_nul(&sa);

  /* set the initial d mode */
  mode = nredir->flag & (R_IN | R_OUT);

  /* additional redirection mode */
  nredir->flag |= rfl;

  TRACE(TRACE_REDIR,
        "eval",
        trace_int("fd", nredir->fdes),
        trace_flags("flag", nredir->flag, trace_redir_flags, 9),
        trace_str("target", sa.s),
        trace_int("preallocated", d != NULL));

  /* a persistent "exec <file" replaces fdtable[n] destructively: fdtable_newfd() below
   * closes the descriptor the old entry owned, so a failing open() after that would lose it
   *   "exec <_no_such_file_"  ->  an interactive shell must keep its stdin
   * so open the file first and hand the descriptor over in redir_open(). */
  if(d == NULL && (nredir->flag & R_ACT) == R_OPEN) {
    preopen = redir_preopen(nredir, &sa);

    if(!fd_ok(preopen)) {
      sh_error_errno(sa.s);
      stralloc_free(&sa);
      return 1;
    }
  }

  /* "[n]<&n" / ">&n" with the same number on both sides is a POSIX no-op (dup2(fd, fd)),
   * e.g. ">&1" inside "{ ...; } > file". Find its source before the new entry below
   * overwrites fdtable[n]:
   *   - a dup of another fd's entry (followed through ->dup) survives and is copied over
   *   - an entry that owns its descriptor is gone after the overwrite: nothing to copy */
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

    /* the new entry: d if the caller gave one, else fdtable_newfd() */
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
      TRACE(TRACE_REDIR, "dup.self", trace_int("fd", nredir->fdes));
      return 0;
    }
  }

  /* run the action. Each frees sa afterwards, except redir_here(): fd_here() takes
   * over sa->s and frees it when the fd is closed, so sa is not touched again. */
  switch(nredir->flag & R_ACT) {
    case R_OPEN:
      r = redir_open(nredir, &sa, preopen);
      stralloc_free(&sa);
      break;
    case R_HERE: r = redir_here(nredir, &sa); break;
    default:
      r = redir_dup(nredir, &sa, d == NULL);
      stralloc_free(&sa);
      break;
  }

  /*  if(nredir->flag & R_NOW)
      return fdtable_resolve(nredir->d, FDTABLE_MOVE);*/

  TRACE(TRACE_REDIR, "eval.status", trace_int("fd", nredir->fdes), trace_int("status", r));
  return r;
}
