#include "../../lib/stralloc.h"
#include "../../lib/buffer.h"
#include "../fd.h"
#include "../../lib/fmt.h"

static void
fd_freehere(buffer* b) {
  if(b->x) {
    alloc_free(b->x);
    b->x = 0;
  }

  b->a = 0;
}

/* prepare (fd) for reading from a stralloc
 *
 * control of the allocated sa->s member is passed to the buffer
 * (means it will be free'd wenn the (fd) is closed)
 * ----------------------------------------------------------------------- */
void
fd_here(struct fd* fd, stralloc* sa) {
  fd->name = "<here>";
  fd->mode = FD_HERE;

  buffer_fromsa(&fd->rb, sa);
  fd->rb.deinit = &fd_freehere;
  fd->e = -1;
}
