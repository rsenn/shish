#include <unistd.h>
#include "fd.h"
#include "fdtable.h"
#include "sh.h"

/* create a pipe 
 * ----------------------------------------------------------------------- */
int fd_pipe(struct fd *fd) {
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
  if(FD_ISWR(fd)) {
    fd_setfd(fd, p[1]);
    e = p[0];
  } else {
    fd_setfd(fd, p[0]);
    e = p[1];
  }
  
  return e;
}
