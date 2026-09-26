#include "../trace.h"
#include "../fd.h"
#include "../fdtable.h"

/* should always be called just after the kernel gives you a new
 * file handle. it is to track the lowest available file descriptor
 * to optimize redirections.
 * ----------------------------------------------------------------------- */
void
fdtable_track(int n, int flags) {
  TRACE(TRACE_FDTABLE, "track", trace_int("n", n), trace_hex("flags", flags), trace_int("expected", fd_expected));

  if(n < fd_expected)
    fd_expected = n;

  if(n > fd_expected)
    fdtable_unexpected(fd_expected, n, flags);

  fdtable_up();
}
