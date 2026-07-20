#include "../../lib/buffer.h"
#include "../fd.h"

/* prepare fd for reading from a string
 * ----------------------------------------------------------------------- */
void
fd_string(struct fd* d, const char* s, size_t len) {
  d->mode = (d->mode & FD_FREE) | FD_STRING;
  d->name = "<string>";

  buffer_fromstr(d->r, (char*)s, len);
}
