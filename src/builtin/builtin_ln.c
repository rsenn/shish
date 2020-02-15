#include "../builtin.h"
#include "../fd.h"
#include "../../lib/shell.h"
#include <sys/stat.h>
#include <unistd.h>

/* output stuff
 * ----------------------------------------------------------------------- */
int
builtin_ln(int argc, char** argv) {
  int c, ret;
  int symbolic = 0, force = 0, verbose = 0;
  const char *src = 0, *dst = ".";

  /* check options */
  while((c = shell_getopt(argc, argv, "fsv")) > 0) {
    switch(c) {
      case 's': symbolic = 1; break;
      case 'f': force = 1; break;
      case 'v': verbose = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  if(argv[optind]) {
    src = argv[optind++];
    if(argv[optind]) {
      dst = argv[optind++];
    }
  }

  ret = (symbolic ? link : symlink)(src, dst);

  if(ret == -1) {
    builtin_error(argv, dst);
    return 1;
  }

  return 0;
}
