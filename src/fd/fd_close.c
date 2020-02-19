#include "../fd.h"
#include "../fdstack.h"
#include "../fdtable.h"

/* flush and close buffers and free the associated ressources
 * ----------------------------------------------------------------------- */
void
fd_close(struct fd* fd) {

  if(!(fd->mode & FD_DUP)) {
    /* update lowest fd if we're below */
    if(fd->e >= 0 && fd->e < fd_exp)
      fd_exp = fd->e;

    if(fd_list[fd->e] == fd)
      fd_list[fd->e] = 0;
  }

  /* when the buffer is opened for writing
     we have to flush it to not loose any data */
  if(D_ISWR(fd))
    buffer_flush(&fd->wb);

  /* set the filedescriptor (which is in fact a stralloc *) on
     stralloc redirections to -1 so they aren't accidentally passed
     to close() */
  if(fd->wb.op == (ssize_t(*)())stralloc_write)
    fd->wb.fd = -1;

  /* if the buffers belong to this (fd) we close them
     don't close twice if we also have a writing buf */
  if(fd->rb.fd != fd->wb.fd)
    buffer_close(&fd->rb);

  buffer_close(&fd->wb);

  /* if the buffer space was temporary then set it to NULL
     so this space isn't used below the current stack level */
  if(fd->mode & FD_TMPBUF) {
    fd->rb.x = 0;
    fd->rb.a = 0;
    fd->wb.x = 0;
    fd->wb.a = 0;
  }

  /* fd is only really closed if there are no duplicates */
  fdstack_unref(fd);
}
