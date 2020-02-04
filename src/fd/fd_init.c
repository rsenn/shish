#include "fd.h"
#include "fdtable.h"
#include "sh.h"
#include "windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* initialize an (fd) struct by setting the defaults to all members
 *
 * (except for the links which are initialized on fdtable_link())
 * ----------------------------------------------------------------------- */
void
fd_init(struct fd* fd, int n, int flags) {
  /* (re-)initialize things */
  fd->mode = flags;
  fd->name = NULL;
  fd->n = n;
  fd->e = -1;
  fd->dup = NULL;
  fd->dev = 0;
  /*  fdrefc = 0;*/

  fd->r = &fd->rb;
  fd->w = &fd->wb;

  buffer_init(fd->r, (buffer_op_proto*)&read, fd->e, NULL, 0);
  buffer_init(fd->w, (buffer_op_proto*)&write, fd->e, NULL, 0);
}
