#ifdef DEBUG_OUTPUT
#include "../../lib/buffer.h"
#include "../fd.h"
#include "../fdtable.h"

void
fdtable_dump(buffer* b) {
  int i;

  buffer_puts(b,
              "  fd  name   level read-buffer                          "
              "write-buffer                        flags\n");
  buffer_puts(b,
              "----------------------------------------------------------------"
              "--------------------------------------------\n");
  buffer_flush(b);

  fdtable_foreach(i) { fd_dump(fdtable[i], b); }
  buffer_flush(b);
}
#endif /* DEBUG_OUTPUT */
