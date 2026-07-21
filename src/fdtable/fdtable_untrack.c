#include "../fd.h"
#include "../fdtable.h"

/* should be called just after a kernel fd is close()d directly, i.e.
 * not through fd_close(). the slot is free again, so it may become the
 * new lowest expected file descriptor -- without this, fd_expected goes
 * stale and every dup() bet in fdtable_dup() misses (one stray
 * dup+close per redirection).
 * ----------------------------------------------------------------------- */
void
fdtable_untrack(int e) {
  if(fd_ok(e) && e < fd_expected)
    fd_expected = e;
}
