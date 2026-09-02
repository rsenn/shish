#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "../fd.h"
#include "../fdtable.h"
#include "../sh.h"
#include "../source.h"
#include "../../lib/windoze.h"
#if WINDOWS_NATIVE
#include <io.h>
#else
#include <unistd.h>
#endif

/* source a file named by an absolute path, silently doing nothing if
 * it doesn't exist/isn't readable -- shared by the $ENV and login-
 * shell (/etc/profile, $HOME/.profile) startup file reads in
 * sh_main.c, neither of which treat a missing file as an error. The
 * access() check has to happen here, before fd_mmap() ever runs:
 * fd_mmap() itself unconditionally reports a failed open() via
 * sh_error_errno(), which is exactly right for every other caller
 * (a real source/exec target that's supposed to exist) but wrong for
 * these two, deliberately-optional files.
 * ----------------------------------------------------------------------- */
void
sh_source(const char* path) {
  struct fd fd;
  struct source src;

  if(access(path, R_OK) != 0)
    return;

  fd_push(&fd, STDSRC_FILENO, FD_READ);
  source_push(&src);
  src.fd = &fd;

  if(!fd_mmap(&fd, path))
    sh_loop();

  source_popfd(&fd);
}
