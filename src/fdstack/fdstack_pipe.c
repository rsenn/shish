#include "fdstack.h"
#include "windoze.h"
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

  b = (unsigned char*)(&fda[n]);

  for(st = fdstack; st; st = st->parent)
    for(fd = st->list; fd; fd = fd->next) {
      /* make pipes for command expansion outputs */
      if((fd->mode & D_SUBST) == D_SUBST) {
        /*      fd->mode |= D_READ;*/

        fd_push(fda, fd->n, D_WRITE | D_FLUSH);
        fd_setbuf(fda, b, D_BUFSIZE / 2);

        e = fd_pipe(fda);
        buffer_init(&fd->rb, (buffer_op_proto*)&read, e, NULL, 0);
        b += D_BUFSIZE / 2;
        fda++;
      }
    }

  return ret;
}
