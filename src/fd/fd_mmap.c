#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../fd.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../../lib/alloc.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* start reading a file on an (fd): memory-mapped when HAVE_MMAP, a
 * plain read(2)-backed buffer otherwise.
 * ----------------------------------------------------------------------- */
int
fd_mmap(struct fd* d, const char* fname) {
  int e;
#if HAVE_MMAP
  int r;
#endif

  d->mode |= FD_FILE;

  if((e = open(fname, O_RDONLY | O_LARGEFILE)) == -1) {
    sh_error_errno(fname);
    return -1;
  }

#if HAVE_MMAP
  fdtable_track(e, FDTABLE_LAZY);

  r = buffer_mmapread_fd(d->r, e);

  close(e);
  fdtable_untrack(e);
  d->r->fd = -1;

  if(r) {
    sh_error_errno(fname);
    return -1;
  }
#else
  /* no mmap: keep the fd open, wire it up to a normal read(2)-based
     buffer via fd_setfd(), and give it its own heap-backed storage
     right away via fd_allocbuf() -- some callers (e.g. builtin_source,
     unlike sh_main.c/sh_fmt.c/sh_parse2ast.c) never follow up with
     their own fd_needbuf()/fd_setbuf(), so this can't rely on that.
     fd_allocbuf() wires the buffer's deinit to free it automatically
     once the fd is torn down. */
  fd_setfd(d, e);
  fd_allocbuf(d, FD_BUFSIZE);
#endif

  /* copy the name: the caller's string isn't guaranteed to outlive
     this (fd) */
  d->name = str_dup(fname);
  d->mode |= FD_FREENAME;

  return 0;
}
