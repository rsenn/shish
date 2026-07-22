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

  /* an existing entry either belongs to our own exact scope (reuse
     it) or to some ancestor scope -- leave fdtable_pos at &fdtable[n]
     either way, so fdtable_link() links a fresh entry *on top* of the
     ancestor's instead of walking into its ->parent slot and linking
     underneath it (which left fdtable[n] still resolving to the
     ancestor's entry, invisible to anything done in the new, deeper
     scope: closing/redirecting a fd already open when shish started,
     from inside a subshell, silently did nothing). Under correct
     push/pop discipline (see fdtable_unlink()) an existing entry can
     never belong to a *deeper* scope than the one creating a new
     entry here, so there's never anything useful further down the
     ->parent chain to walk into. */
  if(vfd && vfd->stack == st)
    return vfd;

  return 0;
}
