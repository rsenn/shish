#include "../builtin.h"
#include "../fd.h"
#include "../../lib/shell.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

/* output stuff
 * ----------------------------------------------------------------------- */
int
builtin_mkdir(int argc, char** argv) {
  int c,ret;
  char* dir;

  /* check options */
  while((c = shell_getopt(argc, argv, "p")) > 0) {
    switch(c) {
      case 'p': break;

      default: builtin_invopt(argv); return 1;
    }
  }

  while((dir = argv[shell_optind++])) {
    ret = mkdir(dir, 0755);
    if(ret == -1) {
      builtin_error(argv, dir);
      return -1;
    }
  }

  return 0;
}
