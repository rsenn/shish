#include "../fd.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* put an already open()ed kernel fd in place of a pending fd entry
 *
 * same as fdtable_open(), minus the open() itself: the file was
 * opened before the entry that owns it came into existence, so that a
 * failure could not destroy what the entry replaces.
 *   "exec <_no_such_file_"  ->  the shell keeps its stdin
 *
 *   struct fd*  d      the entry to install the descriptor on
 *   int         e      the open kernel fd to hand over
 *   int         flags  as for fdtable_open()
 *
 * no fdtable_wish() here: dup2() puts the descriptor exactly where
 * it belongs, so there is nothing to wish for -- and a wish can
 * close/relocate fds, including the one just opened.
 *
 * returns FDTABLE_DONE, or FDTABLE_ERROR if e could not be moved
 * ----------------------------------------------------------------------- */
int
fdtable_openfd(struct fd* d, int e, int flags) {
  /* the file was opened while d->n was still taken, so e is never
     d->n itself: dup2() it into place, after saving whatever entry
     merely *shadowed* by d still holds that descriptor -- dup2()
     closes it in the kernel before anything here could react */
  if(e != d->n) {
    struct fd* occupant = fd_list[d->n];

    /* occupant->n < 0 (e.g. fd_src, STDSRC_FILENO): a negative
       virtual fd is a sentinel slot no ordinary redirection can ever
       address, so fdtable[occupant->n] is always occupant itself --
       "still the live owner of its own slot" can never tell that case
       apart from "already replaced". Treat a negative n as always
       still needed, instead of falling through to steal its kernel fd
       out from under it. */
    if(occupant && occupant != d && !(occupant->mode & FD_CLOSE) &&
       (occupant->n < 0 || occupant != fdtable[occupant->n])) {
      int newfd = dup(occupant->e);

      if(fd_ok(newfd)) {
        /* fd_setfd() -> buffer_default() zeroes rb/wb's "p"/"n" (read
           position / fill count), on the assumption that relocating a
           struct fd means installing it onto a fresh kernel fd with
           nothing buffered yet. Not true here: occupant may be
           actively read from with real, already-buffered-but-not-
           yet-consumed bytes sitting in its buffer memory (fd_src,
           the interpreter's own running-script reader, always is --
           a whole small script is typically read in one buffered
           read(2) call). dup() shares the underlying file offset with
           the original fd, so once that offset reaches EOF (as it
           does the moment the whole script fits in one buffered
           read), any *new* read through the relocated fd returns 0
           immediately -- silently truncating the script right there
           if the buffer's real unconsumed content gets discarded
           along with it. Save and restore it around the call. */
        size_t rp = occupant->rb.p, rn = occupant->rb.n;
        size_t wp = occupant->wb.p, wn = occupant->wb.n;

        fd_setfd(occupant, newfd);

        occupant->rb.p = rp;
        occupant->rb.n = rn;
        occupant->wb.p = wp;
        occupant->wb.n = wn;
      }
    }

    if(dup2(e, d->n) == -1) {
      sh_error_errno(d->name);
      close(e);
      return FDTABLE_ERROR;
    }

    close(e);
    fdtable_untrack(e);
    e = d->n;
  }

  /* track new bottom file descriptor and set the new d */
  fdtable_track(e, flags);
  fd_setfd(d, e);

  d->mode &= ~FD_OPEN;

  return FDTABLE_DONE;
}
