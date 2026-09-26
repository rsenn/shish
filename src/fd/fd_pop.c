#include "../fd.h"
#include "../fdstack.h"
#include "../fdtable.h"
#include "../debug.h"
#include "../trace.h"

/* close fd, unlink from the stack and free its ressources
 * ----------------------------------------------------------------------- */
void
fd_pop(struct fd* fd) {
  /* cleanup paths may pop a redirection that never got as far as
     allocating its fd -- nothing to do then. */
  if(!fd)
    return;

  TRACE(TRACE_FD, "pop", trace_hex("fd", (unsigned long)fd), trace_int("n", fd->n));

  fd_close(fd);
  fdtable_unlink(fd);
  fdstack_unlink(fd);
  fd_free(fd);
}
