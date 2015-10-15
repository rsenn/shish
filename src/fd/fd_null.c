#include <unistd.h>
#include "fd.h"

static ssize_t fd_nullread(int fd, char *b, unsigned long n) {
  return 0;
}

static ssize_t fd_nullwrite(int fd, char *b, unsigned long n) {
  return n;
}

struct fd fd_nullfd = {
  .rb = BUFFER_INIT(fd_nullread, -1, NULL, 0),
  .wb = BUFFER_INIT(fd_nullwrite, -1, NULL, 0),
  .r = &fd_nullfd.rb,
  .w = &fd_nullfd.wb,
};

/* ----------------------------------------------------------------------- */
int fd_null(struct fd *fd) {
  fd->mode |= FD_NULL;
  
  if(FD_ISRD(fd)) fd->r = &fd_nullfd.rb;
  if(FD_ISWR(fd)) fd->w = &fd_nullfd.wb;
  
  fd->name = "/dev/null";
  
  return 0;
}
