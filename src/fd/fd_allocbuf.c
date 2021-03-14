#include "../fd.h"
#include "../../lib/shell.h"

static void
fd_freebuf(buffer* b) {
  shell_free(b->x);
  b->x = NULL;
  b->a = 0;
}

/* allocate buffer space for the (fd).
 * this should only be called when the (fd) really lacks buffer space!
 * ----------------------------------------------------------------------- */
void
fd_allocbuf(struct fd* fd, size_t n) {
  char* p = shell_alloc(n);

  fd_setbuf(fd, p, n);
  if(fd->r->x == p)
    fd->r->deinit = &fd_freebuf;
  if(fd->w->x == p)
    fd->w->deinit = &fd_freebuf;
}
