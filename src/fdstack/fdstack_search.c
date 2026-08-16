#include "../fd.h"
#include "../fdstack.h"
#include "../fdtable.h"

/* searches for virtual file number <n> on fd stack <st>
 * ----------------------------------------------------------------------- */
struct fd*
fdstack_search(struct fdstack* st, int n) {
  struct fd* vfd;

  fdtable_pos = &fdtable[n];
  vfd = *fdtable_pos;

  /* an existing entry is either our own exact scope (reuse it) or an
     ancestor's. Either way leave fdtable_pos at &fdtable[n], so a
     fresh entry links on top of the ancestor's rather than under it. */
  if(vfd && vfd->stack == st)
    return vfd;

  return 0;
}
