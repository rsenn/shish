#if defined(DEBUG_OUTPUT) && (defined(DEBUG_FDTABLE) || defined(DEBUG_FDSTACK) || defined(DEBUG_FD))
#include "../../lib/buffer.h"
#include "../debug.h"
#include "../fd.h"
#include "../fdtable.h"

void
fdtable_dump(buffer* b) {
  int i;

  buffer_puts(b, "  fd name\t\t\t\tlevel  e  mode\t\t\t\t   buffer(s)\n");

  fdtable_foreach(i) {
    struct fd* fd;
    buffer_puts(b, COLOR_DARKGRAY);
    buffer_putns(b, "-", 138);
    buffer_puts(b, COLOR_NONE);
    buffer_putnlflush(b);

    for(fd = fdtable[i]; fd; fd = fd->parent) {
      fd_dump(fd, b);
      buffer_putnlflush(b);
    }
  }

  buffer_putnlflush(b);
}
#endif /* DEBUG_OUTPUT */
