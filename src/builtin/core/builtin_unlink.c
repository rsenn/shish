#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/unix.h"
#include "config.h"
#include <unistd.h>

const char help_unlink[] = "    Remove one file, calling unlink(2) directly.\n"
                           "\n"
                           "    file            the file to remove (not a directory)\n";

int
builtin_unlink(int argc, char* argv[]) {
  int c;

  while((c = shell_getopt(argc, argv, "")) > 0) {
    builtin_invopt(argv);
    return 1;
  }

  if(argc - shell_optind < 1) {
    builtin_errmsg(argv, "missing file operand", NULL);
    return 1;
  }

  if(argc - shell_optind > 1) {
    builtin_errmsg(argv, argv[shell_optind + 1], "extra operand");
    return 1;
  }

  if(unlink(argv[shell_optind]) == -1) {
    builtin_error(argv, argv[shell_optind]);
    return 1;
  }

  return 0;
}
