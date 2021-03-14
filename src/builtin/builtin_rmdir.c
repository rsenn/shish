#include "../builtin.h"
#include "../fd.h"
#include "../../lib/shell.h"
#include "../../lib/scan.h"
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

/* output stuff
 * ----------------------------------------------------------------------- */
int
builtin_rmdir(int argc, char* argv[]) {
  int c, ret;
  int verbose = 0, force = 0, parents = 0;
  char* p;

  /* check options */
  while((c = shell_getopt(argc, argv, ":vfr")) > 0) {
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
    }
    if(verbose) {
      buffer_putm_internal(fd_out->w, "removed dir '", p, "'", 0);
      buffer_putnlflush(fd_out->w);
    }
  }
  return 0;
}
