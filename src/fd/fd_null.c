#include "../fd.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* mark an (fd) entry as closed (">&-" / "<&-")
 *
 * neither FD_READ nor FD_WRITE stay set, so a later fd_dup() against
 * this entry fails exactly like it would against a real closed
 * descriptor (see its FD_ISRD()/FD_ISWR() checks) -- this is what
 * makes a builtin's own I/O (which never goes through the fdtable,
 * see fdtable_resolve()) correctly fail instead of being silently
 * redirected to a null sink as before. The real, possibly still-open
 * kernel descriptor at this fd's number is only closed for real once
 * fdtable_resolve() forces this entry ahead of an execve() (see its
 * FD_NULL check) -- nothing here touches it eagerly, so a plain
 * (non-"exec") ">&-" on a builtin still leaves the original
 * descriptor intact for whatever restores it once the command's fd
 * frame is popped.
 * ----------------------------------------------------------------------- */
int
fd_null(struct fd* fd) {
  fd->mode &= ~(FD_READ | FD_WRITE);
  fd->mode |= FD_NULL;

  return 0;
}
