#include "../fd.h"
#include "../fdstack.h"
#include "../fdtable.h"
#include "../debug.h"
#include <unistd.h>
#include <fcntl.h>

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
  if(fd->wb.op == (ssize_t (*)())(void*)&stralloc_write)
    fd->wb.fd = -1;

  /* the buffers may sit on kernel fds of their own that never became
     the fd's effective descriptor (fd->e stays -1). buffer_close()
     below really close()s those, so untrack them in the fdtable too. */
  if(fd->rb.fd != fd->wb.fd && fd->rb.fd > 2)
    fdtable_untrack(fd->rb.fd);

  if(fd->wb.fd > 2)
    fdtable_untrack(fd->wb.fd);

  /* a real kernel fd is only fd's to close if fd_list[] still shows fd
     as its registered owner. A shadowed struct being reaped here can
     carry a real fd number (via rb.fd/wb.fd, independent of fd->e --
     see the FD_DUP case above and the here-doc/subst case below) that
     a newer struct has since claimed for real; closing it again here
     would sever that struct's own descriptor instead of fd's. Neuter
     fd's own copy first so the buffer_close() calls below become
     no-ops for it. */
  if(fd_ok(fd->rb.fd) && fd_list[fd->rb.fd] && fd_list[fd->rb.fd] != fd)
    fd->rb.fd = -1;

  if(fd_ok(fd->wb.fd) && fd_list[fd->wb.fd] && fd_list[fd->wb.fd] != fd)
    fd->wb.fd = -1;

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
