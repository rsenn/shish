#include "../builtin.h"
#include "../fdtable.h"
#include "../../lib/path.h"
#include "../../lib/str.h"

/* ----------------------------------------------------------------------- */
const char help_basename[] =
    "    Strip directory components from a path.\n"
    "\n"
    "    path            print PATH's last component (after the final '/')\n"
    "    suffix          also strip this suffix, unless it's the whole of\n"
    "                    the resulting name (e.g. \"a.txt\" \".txt\" -> \"a\")\n";

int
builtin_basename(int argc, char* argv[]) {
  char* base;
  const char* suffix;
  size_t blen, slen;

  if(!argv[shell_optind]) {
    builtin_errmsg(argv, "too few arguments", NULL);
    return 1;
  }

  base = path_basename(argv[shell_optind]);
  suffix = argv[shell_optind + 1];

  if(suffix && *suffix) {
    blen = str_len(base);
    slen = str_len(suffix);

    if(slen < blen && !str_diff(base + blen - slen, suffix))
      base[blen - slen] = '\0';
  }

  buffer_puts(fd_out->w, base);
  buffer_putnlflush(fd_out->w);
  return 0;
}
