#include "../fdstack.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* establishs pipes across parent/child for stralloc fds
 * ----------------------------------------------------------------------- */
int
fdstack_pipe(unsigned int n, struct fd* fda) {
  int e;
  struct fd* fd;
  struct fdstack* st;
  unsigned char* b;
  unsigned int ret = 0;
  int depth = 0;

  b = (unsigned char*)(&fda[n]);

  for(st = fdstack; st; st = st->parent) {

    for(fd = st->list; fd; fd = fd->next) {
      /* make pipes for command expansion outputs */
      if((fd->mode & FD_SUBST) == FD_SUBST) {
        /*      fd->mode |= FD_READ;*/

        fd_push(fda, fd->n, FD_WRITE | FD_FLUSH);
        fd_setbuf(fda, b, FD_BUFSIZE / 2);

        e = fd_pipe(fda);
        buffer_init(&fd->rb, (buffer_op_proto*)&read, e, NULL, 0);
        fd->r = 0;
        b += FD_BUFSIZE / 2;

        buffer_puts(buffer_2, "fdstack depth ");
        buffer_putlong(buffer_2, depth);
        buffer_puts(buffer_2, " fda { n=");
        buffer_putlong(buffer_2, fda->n);
        buffer_puts(buffer_2, ", e=");
        buffer_putlong(buffer_2, fda->e);
        buffer_puts(buffer_2, " }, e=");
        buffer_putlong(buffer_2, e);
        buffer_putnlflush(buffer_2);
        fda++;
      }
    }
    depth++;
  }

  return ret;
}
