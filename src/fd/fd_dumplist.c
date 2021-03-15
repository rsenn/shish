#include "../../lib/buffer.h"
#include "../fd.h"

void
fd_dumplist(buffer* b) {
  int i;

  buffer_puts(b,
              "  fd  name     level     read-buffer                          "
              "   write-buffer\n");
  buffer_puts(b,
              "----------------------------------------------------------------"
              "--------------------------------------------\n");

  fd_foreach(i) fd_dump(fd_list[i], b);
}
