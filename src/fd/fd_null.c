#include "fd.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

static ssize_t
fd_nullread(int fd, char* b, unsigned long n) {
  return 0;
}

static ssize_t
fd_nullwrite(int fd, char* b, unsigned long n) {
  return n;
}

struct fd fd_nullfd = {0,
                       0,
                       0,
                       "/dev/null",
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       0,
                       0,
                       BUFFER_INIT(fd_nullread, -1, NULL, 0),
                       BUFFER_INIT(fd_nullwrite, -1, NULL, 0),
                       NULL,
                       NULL};

/* ----------------------------------------------------------------------- */
int
fd_null(struct fd* fd) {
  fd->mode |= D_NULL;

  if(D_ISRD(fd))
    fd->r = &fd_nullfd.rb;
  if(D_ISWR(fd))
    fd->w = &fd_nullfd.wb;

  //  fd->name = "/dev/null";

  return 0;
}
