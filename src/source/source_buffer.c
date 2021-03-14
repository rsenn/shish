#include "../fd.h"
#include "../source.h"

/* ----------------------------------------------------------------------- */
void
source_buffer(struct source* src, struct fd* fd, const char* x, size_t n) {
  fd_push(fd, STDSRC_FILENO, FD_READ);
  fd_string(fd, x, n);
  source_push(src);
}
