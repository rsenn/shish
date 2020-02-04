#include "fdstack.h"

/* returns how many pipes we have to establish from fdstack to fdstack->parent
 * supply D_SUBST, D_HERE or both of them
 * ----------------------------------------------------------------------- */
unsigned int
fdstack_npipes(int mode) {
  struct fd* fd;
  struct fdstack* st;
  unsigned int ret = 0;

  for(st = fdstack; st; st = st->parent)
    for(fd = st->list; fd; fd = fd->next)
      if((fd->mode & mode) == D_SUBST || (fd->mode & mode) == D_HERE)
        ret++;

  return ret;
}
