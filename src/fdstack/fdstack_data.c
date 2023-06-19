#include "../fdstack.h"
#include "../debug.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* sends down here-doc data to pipes and reads command expansions from pipes
 * ----------------------------------------------------------------------- */
int
fdstack_data(void) {
  struct fdstack* st;
#if 0 // defined(DEBUG_OUTPUT) && defined(DEBUG_FDSTACK) && defined(DEBUG_FD) &&
      // !defined(SHFORMAT) && !defined(SHPARSE2AST)
  fdstack_dump(fd_err->w);
#endif
  // fdtable_dump(fd_err->w);

  for(st = fdstack; st; st = st->parent) {
    struct fd* fd;
    for(fd = st->list; fd; fd = fd->next) {

      /* read from the child and put it into output subst buffer */
      if((fd->mode & FD_SUBST) == FD_SUBST) {
        ssize_t n;
        char buf[FD_BUFSIZE / 2];

#if defined(DEBUG_OUTPUT) && defined(DEBUG_FDSTACK) && !defined(SHFORMAT) && \
    !defined(SHPARSE2AST)
        buffer_puts(debug_output, "fdstack_data\n");
        fd_dump(fd, debug_output);
        buffer_putnlflush(debug_output);
#endif

        while((n = read(fd->rb.fd, buf, sizeof(buf))) > 0)
          buffer_put(fd->w, buf, n);

        buffer_flush(fd->w);
      }

      /* read from the stralloc and put it to here-doc pipe */
      /*    if((fd->mode & FD_HERE) == FD_HERE)
          {
            while((n = buffer_get(&fd->rb, buf, sizeof(buf))) > 0)
              write(fd->e, buf, n);
          }*/
    }
  }

  return 0;
}
