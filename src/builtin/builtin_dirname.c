#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/str.h"

/* ----------------------------------------------------------------------- */
const char help_dirname[] = "    Strip the last component from a path.\n"
                            "\n"
                            "    path            print PATH with its last component removed\n"
                            "                    (or '.' if PATH has no '/')\n";

int
builtin_dirname(int argc, char* argv[]) {
  if(argc < 2) {
    builtin_errmsg(argv, "too few arguments", NULL);
    return 1;
  }

  for(int i = shell_optind; i < argc; i++) {
    char* path = argv[i];

    if(str_equal(path, "--"))
      continue;

    if(path) {
      for(i = str_len(path) - 1; i >= 0; i--) {
        if(path[i] == '/') {
          path[i] = '\0';
          break;
        }
      }

      if(i < 0) {
        path[0] = '.';
        path[1] = '\0';
      }
    }

    buffer_puts(fd_out->w, path);
    buffer_putnlflush(fd_out->w);
  }

  return 0;
}
