#include "../../lib/uint64.h"
#include "../../lib/buffer.h"
#include "../../lib/windoze.h"
#include "../fdstack.h"
#include "../debug.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* establishs pipes across parent/child for stralloc fds
 * ----------------------------------------------------------------------- */
int
fdstack_pipe(unsigned int n, struct fd* fds) {
  struct fdstack* st;
  int ret = 0, depth = 0;
  unsigned char* b;

  b = (unsigned char*)(&fds[n]);

  for(st = fdstack; st; st = st->parent) {
    struct fd* fd;

    for(fd = st->list; fd; fd = fd->next) {
      /* make pipes for command expansion outputs */
      if((fd->mode & FD_SUBST) == FD_SUBST) {
        int e;
        /*      fd->mode |= FD_READ;*/

        fd_push(fds, fd->n, FD_WRITE | FD_FLUSH);
        fd_setbuf(fds, b, FD_BUFSIZE / 2);

        e = fd_pipe(fds);
        buffer_init(&fds->parent->rb, (buffer_op_proto*)(void*)&read, e, NULL, 0);
        // fds->parent->mode |= FD_PIPE;
        fds->parent->r = 0;
        b += FD_BUFSIZE / 2;

#if defined(DEBUG_OUTPUT) && defined(DEBUG_FDSTACK)
        buffer_puts(&debug_buffer, "fdstack_pipe n=");
        buffer_putulong(&debug_buffer, n);
        buffer_puts(&debug_buffer, " fds=");
        buffer_putxlonglong(&debug_buffer, (size_t)fds);
        debug_nl_fl();
#endif
        fds++;
        ret++;
      }
    }
    depth++;
  }

  return ret;
}
