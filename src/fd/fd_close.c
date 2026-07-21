#include "../fd.h"
#include "../fdstack.h"
#include "../fdtable.h"
#include "../debug.h"
#include <unistd.h>

/* flush and close buffers and free the associated ressources
 * ----------------------------------------------------------------------- */
void
fd_close(struct fd* fd) {

  if(!(fd->mode & FD_DUP)) {
    /* update lowest fd if we're below */
    if(fd->e >= 0 && fd->e < fd_expected)
      fd_expected = fd->e;

    /* fd->e is -1 for fds that never got a real effective descriptor
       (stralloc-backed here-docs, command substitution, ...) */
    if(fd_ok(fd->e) && fd_list[fd->e] == fd)
      fd_list[fd->e] = 0;
  }

  /* when the buffer is opened for writing
     we have to flush it to not loose any data */
  if(FD_ISWR(fd))
    buffer_flush(&fd->wb);

  /* set the filedescriptor (which is in fact a stralloc *) on
     stralloc redirections to -1 so they aren't accidentally passed
     to close() */
  if(fd->wb.op == (ssize_t(*)())(void*)&stralloc_write)
    fd->wb.fd = -1;

  /* the buffers may sit on kernel fds of their own that never became
     the fd's effective descriptor (fd->e stays -1), e.g. the pipe read
     end a command substitution's stralloc-fd reads the child's output
     from. buffer_close() below really close()s those, so tell the
     fdtable -- otherwise fd_expected stays stale forever and every
     dup() bet in fdtable_dup() misses from then on. */
  if(fd->rb.fd != fd->wb.fd && fd->rb.fd > 2)
    fdtable_untrack(fd->rb.fd);

  if(fd->wb.fd > 2)
    fdtable_untrack(fd->wb.fd);

  /* if the buffers belong to this (fd) we close them
     don't close twice if we also have a writing buf */
  if(fd->rb.fd != fd->wb.fd) {
#if defined(DEBUG_OUTPUT) && defined(DEBUG_FD)
    if(fd->rb.fd != -1 && (fd->mode & FD_READ)) {
      buffer_puts(debug_output, COLOR_YELLOW "fd_close" COLOR_NONE " #");
      buffer_putlong(debug_output, fd->rb.fd);
      debug_nl_fl();
    }
#endif
    buffer_close(&fd->rb);
  }

#if defined(DEBUG_OUTPUT) && defined(DEBUG_FD)
  if(fd->wb.fd != -1 && (fd->mode & FD_WRITE)) {
    buffer_puts(debug_output, COLOR_YELLOW "fd_close" COLOR_NONE " #");
    buffer_putlong(debug_output, fd->wb.fd);
    debug_nl_fl();
  }
#endif
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
