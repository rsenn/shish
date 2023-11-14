#include "../fd.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* start memory mapping a file on an (fd)
 * ----------------------------------------------------------------------- */
int
fd_mmap(struct fd* d, const char* fname) {
  int e;
  int r;

  d->mode |= FD_FILE;

  if((e = open(fname, O_RDONLY | O_LARGEFILE)) == -1) {
    sh_error(fname);
    return -1;
  }

  fdtable_track(e, FDTABLE_LAZY);

  r = buffer_mmapread_fd(d->r, e);

  close(e);
  d->r->fd = -1;

  if(r) {
    sh_error(fname);
    return -1;
  }

  d->name = (char*)fname;

  return 0;
}
