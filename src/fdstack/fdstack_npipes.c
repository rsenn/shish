#include "../fdstack.h"

/* returns how many pipes we have to establish from fdstack to fdstack->parent
 * supply FD_SUBST, FD_HERE or both of them
 * ----------------------------------------------------------------------- */
unsigned int
fdstack_npipes(int mode) {
  struct fd* fd;
  struct fdstack* st;
  unsigned int ret = 0;

  /* Walk outward through empty fdstack levels (redirection/function
   * scopes), stopping at the first one that carries an FD_SUBST/
   * FD_HERE target.
   *
   * - Don't walk past a nested command substitution's own target
   *   (e.g. "$(cmd $(cmd | cmd))") into an outer one still further
   *   out -- that outer target is the outer substitution's own job to
   *   wire a pipe for, once it forks.
   *   
   * - Skip FD_DUP'd entries: a plain redirection duplicating an
   *   FD_SUBST/FD_HERE fd (e.g. "2>&1" on a substitution's fd 1)
   *   copies the same mode bits without being its own target. Counting
   *   it here would stop the walk one level early, before the real
   *   target, leaving it without a pipe.
   *   */
  for(st = fdstack; st; st = st->parent) {
    unsigned int here = 0;

    for(fd = st->list; fd; fd = fd->next)
      if(!(fd->mode & FD_DUP) && ((fd->mode & mode) == FD_SUBST || (fd->mode & mode) == FD_HERE))
        here++;

    ret += here;

    if(here)
      break;
  }

  return ret;
}
