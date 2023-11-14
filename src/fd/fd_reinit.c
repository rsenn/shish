#include "../fd.h"
#include "../../lib/buffer.h"
#include "../../lib/shell.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* reinitialize an (fd) struct
 *
 * (except for the links which are initialized on fdtable_link())
 * ----------------------------------------------------------------------- */
struct fd*
fd_reinit(struct fd* d, int mode) {
  /* unset the name, and if it was allocated: free it */
  if(d->name) {
    if(d->mode & FD_FREENAME)
      alloc_free((char*)d->name);

    d->name = NULL;
  }

  fd_close(d);

  /* re-initialize things */
  d->mode &= FD_FREE;
  d->mode |= mode;

  d->dup = NULL;
  d->dev = 0;
  d->e = -1;

  d->r = &d->rb;
  d->w = &d->wb;

  buffer_default(&d->rb, (buffer_op_proto*)(void*)&read);
  buffer_default(&d->wb, (buffer_op_proto*)(void*)&write);

  return d;
}
