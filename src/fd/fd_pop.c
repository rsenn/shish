#include "../fd.h"
#include "../fdstack.h"
#include "../fdtable.h"
#include "../debug.h"

/* close fd, unlink from the stack and free its ressources
 * ----------------------------------------------------------------------- */
void
fd_pop(struct fd* fd) {

#if defined(DEBUG_OUTPUT) && defined(DEBUG_FD)
  buffer_puts(debug_output, COLOR_YELLOW "fd_pop" COLOR_NONE " 0x");
  buffer_putptr(debug_output, fd);
  debug_nl_fl();
#endif

  fd_close(fd);
  fdtable_unlink(fd);
  fdstack_unlink(fd);
  fd_free(fd);
}
