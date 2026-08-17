#include "../fd.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* mark an fd entry as closed (">&-" / "<&-").
 *
 * - clears FD_READ/FD_WRITE, so fd_dup() against this entry fails
 *   like it would against a real closed descriptor.
 * - doesn't touch the underlying kernel descriptor: that's only
 *   closed for real once fdtable_resolve() forces this entry ahead
 *   of an execve(), so a plain (non-"exec") ">&-" on a builtin
 *   leaves it intact for restoration once the fd frame is popped.
 *
 *   struct fd*  fd   entry to mark closed
 * ----------------------------------------------------------------------- */
int
fd_null(struct fd* fd) {
  fd->mode &= ~(FD_READ | FD_WRITE);
  fd->mode |= FD_NULL;

  return 0;
}
