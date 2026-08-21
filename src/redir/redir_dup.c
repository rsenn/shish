#include "../exec.h"
#include "../expand.h"
#include "../fd.h"
#include "../fdtable.h"
#include "../redir.h"
#include "../../lib/scan.h"

/* do a dup-redirection
 *
 *   struct nredir*  nredir      the redirection being evaluated
 *   stralloc*       sa          the source operand; caller owns and frees it
 *   int             persistent  nonzero for an "exec" redirection
 * ----------------------------------------------------------------------- */
int
redir_dup(struct nredir* nredir, stralloc* sa, int persistent) {
  int ret;

  /* [n]>&- means closing a file descriptor */
  if(sa->len != 1 || sa->s[0] != '-') {
    int fd = 0;

    scan_uint(sa->s, (unsigned int*)&fd);

    /* a bogus fd number (e.g. "3<&99999") must not reach fd_dup()'s
       fdtable[fd] lookup unchecked */
    if(fd < 0 || fd >= FD_MAX) {
      fd_error(fd, "bad file descriptor");
      return 1;
    }

    /* redir_eval() already turns every self-referencing duplicate it
       can safely into a POSIX no-op; reaching here with fdes == fd
       means it couldn't, so there's nothing left to duplicate from. */
    if(nredir->fdes == fd) {
      fd_error(fd, "self-referring duplicate");
      return 1;
    }

    ret = fd_dup(nredir->fd, fd);

    /* resolve a persistent dup now, not lazily: a later redirection
       in the same list can reinit the struct this pending dup chases.
         "exec >&2 2>/dev/null"  ->  fd 1 would resolve to /dev/null
       - FDTABLE_CLOSE skips fdtable_wish()/fdtable_gap(), which would
         try to relocate this fd out of its own slot
       - not inside "(...)": it never forks, so a real dup2()/close()
         here outlives the subshell while fdtable[] is restored around
         it, leaving a slot whose ->e names someone else's real fd */
    if(ret == 0 && persistent && !exec_subshell_depth) {
      if(fdtable_dup(nredir->fd, FDTABLE_FORCE | FDTABLE_CLOSE) == FDTABLE_ERROR) {
        fd_error(fd, "cannot duplicate");
        ret = 1;
      }
    }
  } else
    ret = fd_null(nredir->fd);

  /*  if(nredir->flag & R_NOW)
      fdtable_dup(nredir->fd, FDTABLE_MOVE);*/

  return ret;
}
