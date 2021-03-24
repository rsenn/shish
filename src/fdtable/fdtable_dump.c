#ifdef DEBUG_OUTPUT
#include "../../lib/buffer.h"
#include "../fd.h"
#include "../fdtable.h"

void
fdtable_dump(buffer* b) {
  int i;

  buffer_puts(b,
              "  fd name                                level  e  mode               "
              "              buffer(s)\n");
  buffer_putnc(b, '-', 108);
  buffer_putnlflush(b);

  fdtable_foreach(i) {
    fd_dump(fdtable[i], b);
    buffer_putnlflush(b);
  }
  buffer_flush(b);
}
#endif /* DEBUG_OUTPUT */
