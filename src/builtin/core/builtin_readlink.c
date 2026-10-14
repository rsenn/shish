#include "builtin_config.h"

#if BUILTIN_READLINK

#include "../../builtin.h"
#include "../../fdtable.h"
#include "../../../lib/shell.h"
#include "../../../lib/unix.h"
#include "../../../lib/path.h"
#include "../../../lib/stralloc.h"
#include <sys/stat.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

const char help_readlink[] = "    Print the value of a symbolic link, or canonicalize a path.\n"
                             "\n"
                             "    -n              do not print the trailing newline (one operand only)\n"
                             "    -f              canonicalize by resolving every symlink;\n"
                             "                    the last component may be missing\n"
                             "    -e              like -f, but every component must exist\n"
                             "    -m              like -f, with no existence requirement\n"
                             "    file            symbolic link(s) to read, or path(s) to\n"
                             "                    canonicalize when -f/-e/-m is given\n"
                             "\n"
                             "    long options: --canonicalize -f, --canonicalize-existing -e,\n"
                             "                  --canonicalize-missing -m, --no-newline -n\n";

/* canonicalizes 'path' (following every symlink, like realpath -P)
 * and applies the existence rule for 'mode' ('e': every component
 * must exist, 'f': all but the last, 'm': none).
 * ----------------------------------------------------------------------- */
static int
readlink_canonicalize(char* argv[], const char* path, int mode, int newline) {
  static stralloc sa;
  struct stat st;

  sa.len = 0;

  if(!path_resolve(path, str_len(path), &sa, 0, 0, 0))
    return builtin_error(argv, (char*)path);

  stralloc_nul(&sa);

  if(mode == 'e' && stat(sa.s, &st) == -1)
    return builtin_error(argv, (char*)path);

  if(mode == 'f') {
    size_t n = path_dirlen(sa.s, sa.len);

    if(n) {
      char save = sa.s[n];

      sa.s[n] = '\0';

      if(stat(sa.s, &st) == -1) {
        sa.s[n] = save;
        return builtin_error(argv, (char*)path);
      }

      sa.s[n] = save;
    }
  }

  buffer_puts(fd_out->w, sa.s);
  newline ? buffer_putnlflush(fd_out->w) : buffer_flush(fd_out->w);
  return 0;
}

int
builtin_readlink(int argc, char* argv[]) {
  int c, mode = 0, ret = 0, i, newline = 1;

  while((c = shell_getopt_long(argc, argv, "femn", "canonicalize=f canonicalize-existing=e canonicalize-missing=m no-newline=n")) > 0) {
    switch(c) {
      case 'f':
      case 'e':
      case 'm': mode = c; break;
      case 'n': newline = 0; break;
      default: return builtin_invopt(argv);
    }
  }

  if(shell_optind >= argc) {
    builtin_errmsg(argv, "missing operand", NULL);
    return 1;
  }

  /* with several operands the newlines are what separates them */
  if(argc - shell_optind > 1)
    newline = 1;

  for(i = shell_optind; i < argc; i++) {
    if(mode) {
      if(readlink_canonicalize(argv, argv[i], mode, newline))
        ret = 1;

      continue;
    }

    {
      char buf[PATH_MAX];
      ssize_t n = readlink(argv[i], buf, sizeof(buf) - 1);

      if(n == -1) {
        builtin_error(argv, argv[i]);
        ret = 1;
        continue;
      }

      buf[n] = '\0';
      buffer_puts(fd_out->w, buf);
      newline ? buffer_putnlflush(fd_out->w) : buffer_flush(fd_out->w);
    }
  }

  return ret;
}
#endif /* BUILTIN_READLINK */
