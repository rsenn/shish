#include "../builtin.h"
#include "../fdtable.h"
#include "config.h"
#include <unistd.h>

#if defined(HAVE_LINK)

const char help_link[] = "    Create a hard link, calling link(2) directly.\n"
                         "\n"
                         "    file1           existing file to link to\n"
                         "    file2           name of the new link\n";

int
builtin_link(int argc, char* argv[]) {
  if(argc < 3) {
    builtin_errmsg(argv, "missing file operand", NULL);
    return 1;
  }

  if(argc > 3) {
    builtin_errmsg(argv, argv[3], "extra operand");
    return 1;
  }

  if(link(argv[1], argv[2]) == -1) {
    builtin_error(argv, argv[2]);
    return 1;
  }

  return 0;
}
#endif
