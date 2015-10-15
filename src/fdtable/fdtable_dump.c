#ifdef DEBUG
#include "buffer.h"
#include "fdtable.h"
#include "fd.h"

void fdtable_dump(void) {
  int i;

  buffer_puts(fd_out->w, "  fd  name        level read-buffer                          write-buffer                        flags\n");
  buffer_puts(fd_out->w, "------------------------------------------------------------------------------------------------------------\n");
  buffer_flush(fd_out->w);

  fdtable_foreach(i) {
    fd_dump(fdtable[i]);
    buffer_flush(fd_out->w);
  }
}
#endif /* DEBUG */
