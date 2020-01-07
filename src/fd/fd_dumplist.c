#ifdef DEBUG
#include "buffer.h"
#include "fd.h"

void
fd_dumplist(void) {
  int i;

  buffer_puts(
      fdtable[1]->w,
      "  fd  name        level read-buffer                          write-buffer                        flags\n");
  buffer_puts(
      fdtable[1]->w,
      "------------------------------------------------------------------------------------------------------------\n");

  fd_foreach(i) fd_dump(fd_list[i]);
}
#endif /* DEBUG */
