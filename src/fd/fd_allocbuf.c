#include "../../lib/alloc.h"
#include "../fd.h"
#include "../../lib/shell.h"

static void
fd_freebuf(buffer* b) {
  alloc_free(b->x);
  b->x = NULL;
  b->a = 0;
}

/* allocate buffer space for the (fd).
 * this should only be called when the (fd) really lacks buffer space!
 * ----------------------------------------------------------------------- */
void
fd_allocbuf(struct fd* d, size_t n) {
  char* p = alloc(n);

  fd_setbuf(d, p, n);

  if(d->r->x == p)
    d->r->deinit = &fd_freebuf;

  if(d->w->x == p)
    d->w->deinit = &fd_freebuf;
}
