#include "../fd.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../../lib/windoze.h"

#if WINDOWS_NATIVE
#define pipe(f) _pipe(f, 0, 0)
#else
#include <unistd.h>
#endif

/* create a pipe
 * ----------------------------------------------------------------------- */
int
fd_pipe(struct fd* fd) {
  int p[2];
  int e;

  /* try to create a pipe and return on error */
  if(pipe(p) == -1) {
    sh_error("pipe creation failed");
    return -1;
  }

  /* create new (fd) for the pipe */
  fd->name = "<pipe>";
  fd->mode |= FD_PIPE;

  fdtable_track(p[0], FDTABLE_LAZY);
  fdtable_track(p[1], FDTABLE_LAZY);

  /* set up the file descriptors */
  if(D_ISWR(fd)) {
    fd_setfd(fd, p[1]);
    /*  fd->rb.fd = p[0];
     fd->r = 0; */
    e = p[0];
  } else {
    fd_setfd(fd, p[0]);
    /*    fd->wb.fd = p[1];
       fd->w = 0; */
    e = p[1];
  }

  /*   buffer_puts(buffer_2, "fd#");
    buffer_putlong(buffer_2, fd->n);
    buffer_puts(buffer_2,
                (fd->mode & FD_READ)
                    ? "FD_READ"
                    : (fd->mode & FD_WRITE)
                          ? "FD_WRITE"
                          : (fd->mode & FD_READWRITE) == FD_READWRITE ? "FD_READWRITE" : "");
    buffer_puts(buffer_2, " e=");
    buffer_putlong(buffer_2, fd->e);
   */
  fd_dump(fd, buffer_2);
  buffer_putnlflush(buffer_2);

  return e;
}
