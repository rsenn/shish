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
builtin_chmod(int argc, char** argv) {
int c, ret;
unsigned int mode;
  stralloc path;
  int verbose = 0;
  char* p;
  size_t i;

  /* check options */
  while((c = shell_getopt(argc, argv, "v")) > 0) {
    switch(c) {
      case 'v': verbose = 1; break;
      default: builtin_invopt(argv); return 1;
    }
  }

  p = argv[shell_optind++];
  if(!scan_8int(p, &mode)) {
  }

  while((p = argv[shell_optind++])) {
    ret = chmod(p, mode);
    if(ret == -1) {
      builtin_error(argv, p);
      return 1;
    }
    if(verbose) {
      buffer_putm_internal(fd_out->w, "changed mode of '", p, "'", 0);
      buffer_putnlflush(fd_out->w);
    }
  }
  return 0;
}
