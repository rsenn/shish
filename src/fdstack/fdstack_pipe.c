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
  int ret = 0;
  unsigned char* b;

  b = (unsigned char*)(&fds[n]);

  /* stop at the first fdstack level that actually has a match, same
     as fdstack_npipes() (which "n" came from) -- not an unbounded
     walk all the way out; see its comment for why. */
  for(st = fdstack; st; st = st->parent) {
    struct fd* fd;
    int found = 0;

    for(fd = st->list; fd; fd = fd->next) {
      /* make pipes for command expansion outputs */
      if((fd->mode & FD_SUBST) == FD_SUBST) {
        int e;
        /*      fd->mode |= FD_READ;*/

        fd_push(fds, fd->n, FD_WRITE | FD_FLUSH);
        fd_setbuf(fds, b, FD_BUFSIZE / 2);

        e = fd_pipe(fds);

        /* wire the read end back to "fd" itself -- the struct our
           scan actually matched -- not "fds->parent" (whatever
           currently happens to occupy fd->n's slot at push time).
           Those coincide only when nothing else is shadowing fd yet;
           once something is (e.g. an earlier pipeline stage's own
           inter-stage pipe), fds->parent is that unrelated fd, and
           the real pipe's read end got recorded on a struct that's
           about to be popped and discarded, orphaning fd's own
           buffer with nothing to read from. */
        buffer_init(&fd->rb, (buffer_op_proto*)(void*)&read, e, NULL, 0);
        fd->r = 0;
        b += FD_BUFSIZE / 2;

#if defined(DEBUG_OUTPUT) && defined(DEBUG_FDSTACK)
        buffer_puts(debug_output, "fdstack_pipe n=");
        buffer_putulong(debug_output, n);
        buffer_puts(debug_output, " fds=");
        buffer_putxlonglong(debug_output, (size_t)fds);
        debug_nl_fl();
#endif
        fds++;
        ret++;
        found = 1;
      }
    }

    if(found)
      break;
  }

  return ret;
}
