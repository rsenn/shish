#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_ALLOCA
#include <alloca.h>
#endif

#include "../fd.h"
#include <assert.h>

/* set buffer space for the (fd). the supplied buffer is treated as if
 * it was from alloca(), means that it will get invalid when leaving
 * the current stack level
 *
 * this should only be called when the (fd) really lacks buffer space!
 * ----------------------------------------------------------------------- */
void
fd_setbuf(struct fd* d, void* buf, size_t n) {
  char* p = buf;
  int r = FD_ISRD(d) && !d->r->x;
  int w = FD_ISWR(d) && !d->w->x;

  assert(r || w);

  /* assign buffer space to read buffer */
  if(r) {
    d->r->x = p;
    p += d->r->a = n >> w;
  }

  /* assign buffer space to write buffer */
  if(w) {
    d->w->x = p;
    d->w->a = n >> r;
  }

  /* set the tmpbuf flag so the buffers are set to zero
     if we leave the current stack level! */
  d->mode |= FD_TMPBUF;
}
