#if defined(DEBUG_OUTPUT) && (defined(DEBUG_FDTABLE) || defined(DEBUG_FDSTACK) || defined(DEBUG_FD))

#include "../../lib/buffer.h"
#include "../fd.h"

void
fd_dumplist(buffer* b) {
  int i;

  buffer_puts(b, "  fd name             level  e  mode                      buffer(s)\n");
  buffer_putnc(b, '-', 108);
  buffer_putnlflush(b);

  fd_foreach(i) {
    fd_dump(fd_list[i], b);
    buffer_putnlflush(b);
  }
}
#endif /* DEBUG_OUTPUT */
