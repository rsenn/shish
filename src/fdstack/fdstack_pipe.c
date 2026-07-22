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
      /* make pipes for command expansion outputs -- skip a plain
         redirection duplicating an already-FD_SUBST fd (e.g. "2>&1"
         duplicating a command substitution's fd 1 target): it's not
         its own independent target, just an alias that resolves via
         its own ->dup chain once the real owner below gets its pipe
         wired up; see fdstack_npipes()'s matching comment for the
         full reasoning. */
      if(!(fd->mode & FD_DUP) && (fd->mode & FD_SUBST) == FD_SUBST) {
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

        /* fd->r intentionally still points at &fd->rb (not NULL):
           fdstack_data() (which actually drains the pipe) reads via
           fd->rb.fd directly, bypassing fd->r/buffer_get() entirely
           -- .rb has no backing storage allocated yet (the NULL/0
           above), so nothing must try to buffer_get() through it as
           a normal buffered read. But fdstack_unref(), triggered when
           this fd is later popped while a duplicate of it (e.g.
           "2>&1" on this same target) is still alive, byte_copies
           *fd->r into the surviving duplicate to hand off the real
           pipe fd -- a NULL fd->r there is a segfault, not a no-op. */

        /* "fd" (the fd_subst()/here-doc target) is now the *read*
           side, holding no writable descriptor of its own -- "fds",
           just pushed, is the *write* side that actually got a real
           pipe fd. Any other already-evaluated redirection that dups
           "fd" (e.g. "2>&1" duplicating a command substitution's fd 1
           target, captured back when eval_simple_command's redirect
           loop ran, well before exec_program() ever calls in here)
           still has its ->dup pointer aimed at "fd" -- repoint it at
           "fds" instead, or it would resolve through "fd"'s own
           never-assigned effective fd (-1) once actually dup2()'d for
           the child, instead of the pipe the write side (and thus the
           child) is supposed to share. */
        {
          struct fdstack* dst;
          struct fd* dfd;

          for(dst = fdstack; dst; dst = dst->parent)
            for(dfd = dst->list; dfd; dfd = dfd->next)
              if(dfd->dup == fd)
                dfd->dup = fds;
        }

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
