#include "../trace.h"
#include "../fd.h"
#include "../fdstack.h"
#include "../fdtable.h"

/* push an (fd) to the top fdtable
 * ----------------------------------------------------------------------- */
struct fd*
fd_push(struct fd* d, int n, int mode) {

  TRACE(TRACE_FD, "push", trace_int("n", n), trace_hex("mode", mode), trace_int("level", fdstack->level));

  fd_init(d, n, mode);

  fdtable_pos = &fdtable[n];

  fdtable_link(d);
  fdstack_link(fdstack, d);

  return d;
}
