#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/str.h"

/* ----------------------------------------------------------------------- */
const char help_dirname[] = "    Strip the last component from a path.\n"
                            "\n"
                            "    path            print PATH with its last component removed\n"
                            "                    (or '.' if PATH has no '/')\n";

int
builtin_dirname(int argc, char* argv[]) {
  int i = 1;
  const char* path;
  size_t n;

  if(argc > 1 && !str_diff(argv[1], "--"))
    i++;

  if(argc - i != 1) {
    builtin_errmsg(argv, argc - i < 1 ? "too few arguments" : "too many arguments", NULL);
    return 1;
  }

  path = argv[i];
  n = str_len(path);

  /* "a/b//" -> "a/b/" -> "a/" -> "a" */
  while(n > 1 && path[n - 1] == '/')
    n--;
  while(n > 0 && path[n - 1] != '/')
    n--;
  while(n > 1 && path[n - 1] == '/')
    n--;

  if(n == 0)
    buffer_puts(fd_out->w, ".");
  else
    buffer_put(fd_out->w, path, n);

  buffer_putnlflush(fd_out->w);
  return 0;
}
