#include "../fd.h"
#include "../fdtable.h"
#include "../../lib/windoze.h"

#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

/* drop a here-document to a temporary file
 * ----------------------------------------------------------------------- */
int
fdtable_here(struct fd* d, int flags) {
  int e, state;
  char* x;
  unsigned long n;

  /* wish d->n become the next expected file descriptor */
  state = fdtable_wish(d->n, flags);

  /* the wish may have (recursively) resolved our d already */
  if(d->e == d->n)
    return FDTABLE_DONE;

  /* leave it for now if we're in lazy mode and still pending */
  if(state == FDTABLE_PENDING && (flags & FDTABLE_FD) == FDTABLE_LAZY)
    return state;

  /* maybe we can close the destination d */
  if(state == d->n) {
    close(d->n);
    fdtable_untrack(d->n);
  }

  /* try to create temporary file */
  if((e = fd_tempfile(d)) < 0)
    return e;

  /* write buffer data to temp. file in chunks half of the buffer size */
  n = d->rb.n;
  x = d->rb.x;

  while(n) {
    unsigned long sz = (n > FD_BUFSIZE2 ? FD_BUFSIZE2 : n);
    buffer_put(&d->wb, x, sz);
    n -= sz;
    x += sz;
  }

  buffer_flush(&d->wb);

  /* seek back to the current read position; note there is no portable
     way to actually downgrade an already-open fd to read-only via
     fcntl(F_SETFL) -- access mode isn't one of the flags it can change */
  lseek(e, d->rb.p, SEEK_SET);

  /* initialize the read buffer so we can read from
     the tempfile and destroy the write buffer */
  buffer_init(&d->rb, (buffer_op_proto*)(void*)&read, e, NULL, 0);
  buffer_init(&d->wb, (buffer_op_proto*)(void*)&write, -1, NULL, 0);

  /* its now not longer a stralloc :) */
  d->mode = (d->mode & FD_FREE) | FD_READ;

  if(d->e == d->n || !(flags & FDTABLE_FORCE))
    return FDTABLE_DONE;

  return d->n;
}
