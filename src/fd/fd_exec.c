#include "fd.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* make an (fd) ready for an execve()
 *
 * returns the effective file descriptor
 * ----------------------------------------------------------------------- */
int
fd_exec(struct fd* fd) {
  int tmp = -1;

  /* dump here-doc redirections that are still a stralloc to a
     temporary file */
  if((fd->mode & D_HERE) == D_HERE) {
    if((tmp = fd_tempfile(fd)) >= 0) {
      unsigned long p;

      /* read from the read buf (stralloc)
         and put it into write buf (tempfile) */
      for(p = 0; p < fd->rb.n; p += 128) buffer_put(&fd->wb, &fd->rb.x[p], (fd->rb.n - p > 128) ? 128 : (fd->rb.n - p));

      buffer_flush(&fd->wb);
      buffer_free(&fd->rb);

      /* seek the file back */
      lseek(tmp, (long)0, SEEK_SET);

      /* initialize the read buffer so we can read from the tempfile */
      buffer_init(&fd->rb, (buffer_op_proto*)&read, tmp, NULL, 0);

      /* destroy the write buffer */
      buffer_init(&fd->wb, NULL, -1, NULL, 0);

      /* now we got rid of the stralloc :) */
      fd->mode &= ~D_STRALLOC;
    }
  }

  return tmp;
}
