#include "../fdstack.h"
#include "../job.h"
#include "../sh.h"
#include <errno.h>
#include "../trace.h"
#include "../fdtable.h"
#include "../debug.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#include <poll.h>
#endif

/* sends down here-doc data to pipes and reads command expansions from pipes
 * ----------------------------------------------------------------------- */
int
fdstack_data(void) {
  struct fdstack* st;

  for(st = fdstack; st; st = st->parent) {
    struct fd* fd;

    for(fd = st->list; fd; fd = fd->next) {
      /* read from the child and put it into output subst buffer */
      if((fd->mode & FD_SUBST) == FD_SUBST && (fd->mode & FD_READ)) {
        ssize_t n;
        unsigned long total = 0;
        char buf[FD_BUFSIZE / 2];

        for(;;) {
#if !WINDOWS_NATIVE
          /* with job control a child may stop while it holds the pipe open:
             look for that every so often instead of blocking until EOF */
          if(sh->opts.monitor) {
            struct pollfd pfd = {fd->rb.fd, POLLIN, 0};
            int r = poll(&pfd, 1, 100);

            if(r == 0) {
              job_resume_stopped();
              continue;
            }

            if(r < 0 && errno == EINTR)
              continue;
          }
#endif
          if((n = read(fd->rb.fd, buf, sizeof(buf))) <= 0)
            break;

          buffer_put(fd->w, buf, n);
          total += n;
        }

        TRACE(TRACE_FDSTACK, "data", trace_fd("fd", fd), trace_int("bytes", total));
        (void)total;

        buffer_flush(fd->w);

        /* drop the pipe's read end and FD_READ, so a later command in the
           same substitution ("$(a; b)") counts as an FD_SUBST target again
           in fdstack_npipes() */
        if(fd_ok(fd->rb.fd)) {
          if(fd_list[fd->rb.fd] == fd)
            fd_list[fd->rb.fd] = 0;

          if(fd->rb.fd > 2)
            fdtable_untrack(fd->rb.fd);

          close(fd->rb.fd);
        }

        fd->rb.fd = -1;
        fd->mode &= ~FD_READ;
      }
    }
  }

  return 0;
}

/* in a forked child: close the read ends of the substitution pipes, which only
 * the parent drains. Left open they hold low kernel fds ("exec 3>&1" in
 * "$(...)" wants 3) until fdstack_flatten() at the very end.
 * ----------------------------------------------------------------------- */
void
fdstack_closerd(void) {
  struct fdstack* st;
  struct fd* fd;

  for(st = fdstack; st; st = st->parent)
    for(fd = st->list; fd; fd = fd->next) {
      if((fd->mode & FD_SUBST) != FD_SUBST || !(fd->mode & FD_READ) || !fd_ok(fd->rb.fd))
        continue;

      if(fd_list[fd->rb.fd] == fd)
        fd_list[fd->rb.fd] = 0;

      if(fd->rb.fd > 2)
        fdtable_untrack(fd->rb.fd);

      close(fd->rb.fd);
      fd->rb.fd = -1;
      fd->mode &= ~FD_READ;
    }
}
