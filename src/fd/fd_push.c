#include "../fd.h"
#include "../fdstack.h"
#include "../fdtable.h"

/* push an (fd) to the top fdtable
 * ----------------------------------------------------------------------- */
struct fd*
fd_push(struct fd* d, int n, int mode) {

  fd_init(d, n, mode);

  fdtable_pos = &fdtable[n];

  fdtable_link(d);
  fdstack_link(fdstack, d);

  return d;
}
