#include "../fd.h"
#include "../fdstack.h"
#include "../fdtable.h"
#include "../debug.h"

/* close fd, unlink from the stack and free its ressources
 * ----------------------------------------------------------------------- */
void
fd_pop(struct fd* fd) {
  /* eval_simple_command.c's cleanup path walks every redirection
     parsed for a command and pops each one's ->nredir.fd, even if it
     bailed out (e.g. a readonly-variable assignment failing) before
     ever reaching the loop that actually allocates/assigns those --
     nothing to pop yet for those, unlike a real (fd) that was pushed */
  if(!fd)
    return;

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
