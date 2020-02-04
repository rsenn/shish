#include "fd.h"
#include "../../lib/buffer.h"
#include "fdstack.h"
#include <assert.h>
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* initialize (fd) from file descriptor
 * ----------------------------------------------------------------------- */
int
fd_setfd(struct fd* fd, int e) {
  assert(fd->mode & D_READWRITE);

  /* set the file descriptors on the buffers */
  if(D_ISRD(fd)) {
    buffer_default(&fd->rb, (buffer_op_proto*)&read);
    fd->rb.fd = e;
  }

  if(D_ISWR(fd)) {
    buffer_default(&fd->wb, (buffer_op_proto*)&write);
    fd->wb.fd = e;
  }

  /* track the file descriptor */
  fd->e = e;

  /* update duplicates of fd */
  fdstack_update(fd);

  if(fd_ok(e)) {
    fd_list[e] = fd;

    if(fd_hi <= e)
      fd_hi = e + 1;
    if(fd_lo > e)
      fd_lo = e;
  }

  return e;
}
