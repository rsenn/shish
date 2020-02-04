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
fd_mmap(struct fd* fd, const char* fname) {
  int e;
  int r;

  fd->mode |= D_FILE;

  if((e = open(fname, O_RDONLY | O_LARGEFILE)) == -1) {
    sh_error(fname);
    return -1;
  }

  fdtable_track(e, FDTABLE_LAZY);

  r = buffer_mmapread_fd(fd->r, e);

  close(e);
  fd->r->fd = -1;

  if(r) {
    sh_error(fname);
    return -1;
  }

  fd->name = (char*)fname;

  return 0;
}
