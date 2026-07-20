#include "../fd.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../../lib/alloc.h"
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
    sh_error_errno(fname);
    return -1;
  }

  fdtable_track(e, FDTABLE_LAZY);

  r = buffer_mmapread_fd(d->r, e);

  close(e);
  d->r->fd = -1;

  if(r) {
    sh_error_errno(fname);
    return -1;
  }

  /* copy the name: the caller's string isn't guaranteed to outlive
     this (fd) */
  d->name = str_dup(fname);
  d->mode |= FD_FREENAME;

  return 0;
}
