#include "../fd.h"
#include "../sh.h"
#include "../debug.h"
#include "../../lib/buffer.h"
#include "../fdstack.h"
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
  assert(fd->mode & FD_READWRITE);

  /* set the file descriptors on the buffers */
  if(FD_ISRD(fd)) {
    buffer_default(&fd->rb, (buffer_op_proto*)&read);
    fd->rb.fd = e;
    fd->r = &fd->rb;
  }

  if(FD_ISWR(fd)) {
    buffer_default(&fd->wb, (buffer_op_proto*)&write);
    fd->wb.fd = e;
    fd->w = &fd->wb;
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

#ifdef DEBUG_FD
  if(sh->opts.debug) {
    if(fd->e != -1) {
      buffer_puts(&debug_buffer, COLOR_YELLOW "fd_setfd" COLOR_NONE " #");
      buffer_putlong(&debug_buffer, fd->n);
      buffer_puts(&debug_buffer, " e=");
      buffer_putlong(&debug_buffer, fd->e);
      buffer_puts(&debug_buffer, " mode=");
      buffer_puts(&debug_buffer,
                  (fd->mode & FD_READ) ? "FD_READ" : (fd->mode & FD_WRITE) ? "FD_WRITE" : (fd->mode & FD_READWRITE) == FD_READWRITE ? "FD_READWRITE" : "");

      debug_nl_fl();
    }
  }
#endif
  return e;
}
