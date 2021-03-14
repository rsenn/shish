#include "../source.h"
#include "../fd.h"

/* ----------------------------------------------------------------------- */
void
source_popfd(struct fd* fd) {
  source_pop();
  fd_pop(fd);
}
