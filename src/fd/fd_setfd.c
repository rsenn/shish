#include "fd.h"
#include "buffer.h"
#include "fdstack.h"
#include <assert.h>
#include "windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

struct fd* fd_list[FD_MAX];

/* initialize (fd) from file descriptor
 * ----------------------------------------------------------------------- */
int
fd_setfd(struct fd* fd, int e) {
  assert(fd->mode & FD_READWRITE);

  /* set the file descriptors on the buffers */
  if(FD_ISRD(fd)) {
    buffer_default(&fd->rb, (void*)&read);
    fd->rb.fd = e;
  }

  if(FD_ISWR(fd)) {
    buffer_default(&fd->wb, (void*)&write);
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
