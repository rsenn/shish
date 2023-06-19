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
    buffer_default(&fd->rb, (buffer_op_proto*)(void*)&read);
    fd->rb.fd = e;
    fd->r = &fd->rb;
  }

  if(FD_ISWR(fd)) {
    buffer_default(&fd->wb, (buffer_op_proto*)(void*)&write);
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

#if defined(DEBUG_OUTPUT) && defined(DEBUG_FD)
  if(sh->opts.xtrace) {
    if(fd->e != -1) {
      buffer_puts(debug_output, COLOR_YELLOW "fd_setfd" COLOR_NONE " #");
      buffer_putlong(debug_output, fd->n);
      buffer_puts(debug_output, " e=");
      buffer_putlong(debug_output, fd->e);
      buffer_puts(debug_output, " mode=");
      buffer_puts(debug_output,
                  (fd->mode & FD_READ)                        ? "FD_READ"
                  : (fd->mode & FD_WRITE)                     ? "FD_WRITE"
                  : (fd->mode & FD_READWRITE) == FD_READWRITE ? "FD_READWRITE"
                                                              : "");

      debug_nl_fl();
    }
  }
#endif
  return e;
}
