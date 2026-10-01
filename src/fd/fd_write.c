#include "../fd.h"
#include "../../lib/buffer.h"
#include <errno.h>

/* errno of the last failed write on any fd buffer; a builtin that
 * returned 0 while this is set has to report a write error.
 * ----------------------------------------------------------------------- */
int fd_write_errno = 0;

/* buffer_op_write() that remembers why a write failed
 * ----------------------------------------------------------------------- */
ssize_t
fd_write(int fd, void* buf, size_t len, void* arg) {
  ssize_t r = buffer_op_write(fd, buf, len, arg);

  if(r < 0)
    fd_write_errno = errno;

  return r;
}
