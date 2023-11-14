#include "../fd.h"
#include "../source.h"

/* ----------------------------------------------------------------------- */
void
source_buffer(struct source* s, struct fd* d, const char* x, size_t n) {
  fd_push(d, STDSRC_FILENO, FD_READ);
  fd_string(d, x, n);
  source_push(s);
}
