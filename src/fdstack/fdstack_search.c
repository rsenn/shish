#include "../fd.h"
#include "../fdstack.h"
#include "../fdtable.h"

/* searches for virtual file number <n> on fd stack <st>
 * ----------------------------------------------------------------------- */
struct fd*
fdstack_search(struct fdstack* st, int n) {
  struct fd* vfd;

  for(fdtable_pos = &fdtable[n]; (vfd = *fdtable_pos);
      fdtable_pos = &vfd->parent) {
    if(vfd->stack == st)
      return vfd;

    if(vfd->stack->level >= st->level)
      break;
  }

  return 0;
}
