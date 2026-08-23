#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/shell.h"
#include "../../lib/path.h"
#include "../../lib/stralloc.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

/* -p: having just removed 'sa', keep climbing to its parent, its
 * parent's parent, and so on, rmdir()'ing each as long as they're
 * empty (GNU rmdir -p's "a/b/c" -> "a/b" -> "a" walk). path_right()
 * gives the length before the last component; a value >= sa->len (no
 * separator left) or 0 (root-only path) both mean "nothing left above
 * this", stopping the climb before rmdir("")/rmdir("/"). Returns 0 if
 * the climb ran out of ancestors on its own, 1 on a failed rmdir().
 * ----------------------------------------------------------------------- */
static int
rmdir_parents(char* argv[], stralloc* sa, int verbose, int force) {
  size_t len;

  while((len = path_right(sa->s, sa->len)) > 0 && len < sa->len) {
    sa->len = len;
    stralloc_nul(sa);

    if(rmdir(sa->s) == -1) {
      /* not empty (or anything else) -- climbing further wouldn't
         help either, so this is where -p's chain ends, successfully
         removed ancestors and all */
      builtin_error(argv, sa->s);
      return force ? 0 : 1;
    }

    if(verbose) {
      buffer_putm_internal(fd_out->w, "removed dir '", sa->s, "'", 0);
      buffer_putnlflush(fd_out->w);
    }
  }

  return 0;
}

/* output stuff
 * ----------------------------------------------------------------------- */
const char help_rmdir[] = "    Remove empty directories.\n"
                          "\n"
                          "    -p              also remove now-empty ancestor directories\n"
                          "    -f              ignore errors, keep going\n"
                          "    -v              print each directory removed\n"
                          "    directory       empty directory (or directories) to remove\n";

int
builtin_rmdir(int argc, char* argv[]) {
  int c, ret, verbose = 0, force = 0, parents = 0;
  char* p;

  /* check options */
  while((c = shell_getopt(argc, argv, "vfrp")) > 0) {
    switch(c) {
      case 'v': verbose = 1; break;
      case 'f': force = 1; break;
      case 'p': parents = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  while((p = argv[shell_optind++])) {
    ret = rmdir(p);

    if(ret == -1) {
      builtin_error(argv, p);

      if(!force)
        return 1;

      continue;
    }

    if(verbose) {
      buffer_putm_internal(fd_out->w, "removed dir '", p, "'", 0);
      buffer_putnlflush(fd_out->w);
    }

    if(parents) {
      stralloc sa;
      int failed;

      stralloc_init(&sa);
      stralloc_copys(&sa, p);
      stralloc_nul(&sa);
      failed = rmdir_parents(argv, &sa, verbose, force);
      stralloc_free(&sa);

      if(failed)
        return 1;
    }
  }

  return 0;
}
