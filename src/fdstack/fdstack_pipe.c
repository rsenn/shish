#include "../../lib/uint64.h"
#include "../../lib/buffer.h"
#include "../fdstack.h"
#include "../debug.h"

/* establishs pipes across parent/child for stralloc fds
 * ----------------------------------------------------------------------- */
int
fdstack_pipe(unsigned int n, struct fd* fds) {
  struct fdstack* st;
  int ret = 0;
  unsigned char* b;

  b = (unsigned char*)(&fds[n]);

  /* stop at the first fdstack level with a match, same as
     fdstack_npipes() (which "n" came from). */
  for(st = fdstack; st; st = st->parent) {
    struct fd* fd;
    int found = 0;

    for(fd = st->list; fd; fd = fd->next) {
      /* make pipes for command expansion outputs; skip a plain
         redirection duplicating an already-FD_SUBST fd (e.g. "2>&1"
         duplicating a substitution's fd 1) -- it's just an alias that
         resolves via its ->dup chain once the real owner is wired up. */
      if(!(fd->mode & FD_DUP) && (fd->mode & FD_SUBST) == FD_SUBST) {
        int e;

        fd_push(fds, fd->n, FD_WRITE | FD_FLUSH);
        fd_setbuf(fds, b, FD_BUFSIZE / 2);

        e = fd_pipe(fds);

        /* wire the read end back to "fd" itself, the struct our scan
           matched -- not "fds->parent", which may be an unrelated fd
           already shadowing this slot. */
        buffer_init(&fd->rb, &buffer_op_read, e, NULL, 0);

        /* fd_pipe(fds) above only registers fd_list[]/fd_hi/fd_lo for
           its own end (fds, via fd_setfd()) -- "e" (the other end,
           just wired to "fd" above) never goes through fd_setfd(), so
           fd_list[e] is left NULL. Left that way, a later real fd
           allocation that lands above "e" sees a gap and backfills it
           with a synthetic placeholder (fdtable_unexpected()), and
           fd_close() then finds fd_list[e] pointing at that unrelated
           placeholder instead of "fd" -- it neuters fd->rb.fd to -1
           to avoid double-closing what it thinks is someone else's fd,
           so the real pipe-read end never actually gets close()d
           (fd-subst-read-end-fd-leaks-and-corrupts-later-fd_setfd).
           Register it here the same way fd_setfd() would, without
           calling fd_setfd() itself: fd's write side (fd->wb) is a
           stralloc, not this real fd, and fd_setfd() would clobber
           fd->wb.fd with "e" since fd->mode still carries FD_WRITE
           from FD_SUBST. */
        fd->mode |= FD_READ;
        fd_list[e] = fd;

        if(fd_hi <= e)
          fd_hi = e + 1;

        if(fd_lo > e)
          fd_lo = e;

        /* fd->r stays pointed at &fd->rb: fdstack_data() reads via
           fd->rb.fd directly (bypassing buffer_get()), but
           fdstack_unref() later copies *fd->r into a surviving
           duplicate when this fd is popped, so it must not be NULL. */

        /* "fd" is now the read side with no writable descriptor of
           its own; "fds" (just pushed) is the write side holding the
           real pipe fd. Repoint any redirection that still dups "fd"
           (e.g. "2>&1") at "fds" instead. */
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
