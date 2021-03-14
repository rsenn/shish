#include "../../lib/uint64.h"
#include "../../lib/buffer.h"
#include "../../lib/windoze.h"
#include "../fdstack.h"
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
        buffer_init(&fda->parent->rb, (buffer_op_proto*)&read, e, NULL, 0);
        fda->parent->r = 0;
        b += FD_BUFSIZE / 2;

#ifdef DEBUG_FDSTACK
        buffer_puts(buffer_2, "fdstack_pipe n=");
        buffer_putulong(buffer_2, n);
        buffer_puts(buffer_2, " fda=");
        buffer_putxlonglong(buffer_2, (size_t)fda);
        buffer_putnlflush(buffer_2);
#endif
        fda++;
      }
    }
    depth++;
  }

  return ret;
}
