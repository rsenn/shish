#ifdef WIN32
#include <windows.h>
#include <io.h>
#endif

#include <fcntl.h>
#include "fd.h"
#include "fdtable.h"

/* drop a here-document to a temporary file
 * ----------------------------------------------------------------------- */
int fdtable_here(struct fd *fd, int flags)
{
  int e;
  int state;
  char *x;
  unsigned long n;

  /* wish fd->n become the next expected file descriptor */
  state = fdtable_wish(fd->n, flags);
  
  /* the wish may have (recursively) resolved our fd already */
  if(fd->e == fd->n)
    return FDTABLE_DONE;
  
  /* leave it for now if we're in lazy mode and still pending */
  if(state == FDTABLE_PENDING && (flags & FDTABLE_FD) == FDTABLE_LAZY)
    return state;
  
  /* maybe we can close the destination fd */
  if(state == fd->n)
    close(fd->n);
  
  /* try to create temporary file */
  if((e = fd_tempfile(fd)) < 0)
    return e;

  /* write buffer data to temp. file in chunks half of the buffer size */
  n = fd->rb.n;
  x = fd->rb.x;

  while(n)
  {
    unsigned long sz = (n > FD_BUFSIZE2 ? FD_BUFSIZE2 : n);
    buffer_put(&fd->wb, x, sz);
    n -= sz;
    x += sz;
  }

  buffer_flush(&fd->wb);

  /* make the fd read-only and seek to the current position */
#ifdef F_SETFL
  fcntl(e, F_SETFL, O_RDONLY);
#endif
#ifdef WIN32
  SetFilePointer(e, fd->rb.p, 0, FILE_BEGIN);
#else
  lseek(e, fd->rb.p, SEEK_SET);
#endif

  /* initialize the read buffer so we can read from 
     the tempfile and destroy the write buffer */
  buffer_init(&fd->rb, read, e, NULL, 0);
  buffer_init(&fd->wb, write, -1, NULL, 0);

  /* its now not longer a stralloc :) */
  fd->mode = FD_READ;

  if(fd->e == fd->n || !(flags & FDTABLE_FORCE))
    return FDTABLE_DONE;
  
  return fd->n;
}
