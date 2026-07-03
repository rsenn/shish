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
fd_setfd(struct fd* d, int e) {
  int old_e = d->e;
  assert(d->mode & FD_READWRITE);

  /* set the file descriptors on the buffers */
  if(FD_ISRD(d)) {
    buffer_default(&d->rb, (buffer_op_proto*)(void*)&read);
    d->rb.fd = e;
    d->r = &d->rb;
  }

  if(FD_ISWR(d)) {
    buffer_default(&d->wb, (buffer_op_proto*)(void*)&write);
    d->wb.fd = e;
    d->w = &d->wb;
  }

  /* fd_list[e] should map each live kernel fd to the shish-fd struct
     that currently owns it. When d->e changes, fd_list[old_e] becomes
     stale — fdtable_gap(old_e) would then find a struct whose e is no
     longer old_e and recurse into resolving it, eventually closing a
     kernel fd the caller is still relying on (the cmd-sub pipe path).
     Common trigger: `exec 0<file` runs fdtable_open → fdtable_track
     → fdtable_unexpected, which fills the gap at d->n by reinit'ing d
     to e=0; fdtable_open's final fd_setfd then sets d->e to the open()
     result without ever clearing fd_list[0]. */
  if(old_e != e && fd_ok(old_e) && fd_list[old_e] == d)
    fd_list[old_e] = 0;

  /* track the file descriptor */
  d->e = e;

  /* update duplicates of d */
  fdstack_update(d);

  if(fd_ok(e)) {
    fd_list[e] = d;

    if(fd_hi <= e)
      fd_hi = e + 1;

    if(fd_lo > e)
      fd_lo = e;
  }

#if defined(DEBUG_OUTPUT) && defined(DEBUG_FD)
  if(sh->opts.xtrace) {
    if(d->e != -1) {
      buffer_puts(debug_output, COLOR_YELLOW "fd_setfd" COLOR_NONE " #");
      buffer_putlong(debug_output, d->n);
      buffer_puts(debug_output, " e=");
      buffer_putlong(debug_output, d->e);
      buffer_puts(debug_output, " mode=");
      buffer_puts(debug_output,
                  (d->mode & FD_READ)                        ? "FD_READ"
                  : (d->mode & FD_WRITE)                     ? "FD_WRITE"
                  : (d->mode & FD_READWRITE) == FD_READWRITE ? "FD_READWRITE"
                                                             : "");

      debug_nl_fl();
    }
  }
#endif
  return e;
}
