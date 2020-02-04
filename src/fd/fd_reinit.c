#include "../../lib/buffer.h"
#include "fd.h"
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
fd_reinit(struct fd* fd, int flags) {
  /* unset the name, and if it was allocated: free it */
  if(fd->name) {
    if(fd->mode & D_FREENAME)
      shell_free((char*)fd->name);

    fd->name = NULL;
  }

  fd_close(fd);

  /* re-initialize things */
  fd->mode &= D_FREE;
  fd->mode |= flags;

  fd->dup = NULL;
  fd->dev = 0;
  fd->e = -1;

  fd->r = &fd->rb;
  fd->w = &fd->wb;

  buffer_default(&fd->rb, (buffer_op_proto*)&read);
  buffer_default(&fd->wb, (buffer_op_proto*)&write);

  return fd;
}
