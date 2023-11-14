#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/shell.h"
#include "config.h"
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>

#if defined(HAVE_SYMLINK) && defined(HAVE_LINK)

#ifndef HAVE_LSTAT
#define lstat stat
#endif

/* output stuff
 * ----------------------------------------------------------------------- */
int
builtin_ln(int argc, char* argv[]) {
  int c, is_dir = 0, ret;
  stralloc path;
  int symbolic = 0, force = 0, verbose = 0;
  char *src = 0, *dst = 0;
  size_t len;

  /* check options */
  while((c = shell_getopt(argc, argv, "fsv")) > 0) {
    switch(c) {
      case 's': symbolic = 1; break;
      case 'f': force = 1; break;
      case 'v': verbose = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  c = argc - shell_optind;

  if(c >= 2 && argc > 0) {
    dst = argv[argc - 1];
    argv[argc - 1] = NULL;
  }

  if(c >= 1) {
    struct stat st;

    if(lstat(dst, &st) == 0) {
      is_dir = S_ISDIR(st.st_mode);
    }
  }

  stralloc_init(&path);
  stralloc_copys(&path, dst);
  stralloc_catc(&path, '/');

  len = path.len;

  while((src = argv[shell_optind++])) {
    if(is_dir) {
      path.len = len;
      stralloc_cats(&path, basename(src));
      stralloc_nul(&path);
    }

    unlink(path.s);
    ret = (symbolic ? symlink : link)(src, path.s);

    if(ret == -1) {
      builtin_error(argv, path.s);

      if(!force)
        return 1;
    }

    if(verbose) {
      buffer_putm_internal(fd_out->w, "'", path.s, "' -> '", src, "'", 0);
      buffer_putnlflush(fd_out->w);
    }
  }
  return 0;
}
#endif
