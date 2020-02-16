#include "../shell.h"
#include "../byte.h"
#include "../str.h"
#include "../stralloc.h"

#include <errno.h>
#include <limits.h>

#ifndef PATH_MAX
#if WINDOWS
#include <windows.h>
#endif
#define PATH_MAX MAX_PATH
#endif

extern int shell_canonicalize(const char* path, stralloc* sa, int symbolic);

/* if the <path> is relative and <cwd> is non-null then it is prepended
 * to the path, so it will work like shell_canonicalize, except that
 * relative paths will be resolved to absolute ones.
 * ----------------------------------------------------------------------- */
int
shell_realpath(const char* path, stralloc* sa, int symbolic, stralloc* cwd) {
  /* if its not absolute on the first recursion level then make it so */
  if(*path != '/' && sa->len == 0) {
    char buf[PATH_MAX + 1];

    /* check whether the name fits */
    unsigned long n;
    n = str_len(path);

    if(cwd->len + n + 1 > PATH_MAX) {
      errno = ENAMETOOLONG;
      return 0;
    }

    /* copy current dir */
    byte_copy(buf, cwd->len, cwd->s);
    buf[cwd->len] = '/';
    byte_copy(&buf[cwd->len + 1], n + 1, path);

    /* run canonicalize with the concatenated path */
    return shell_canonicalize(buf, sa, symbolic);
  }

  return shell_canonicalize(path, sa, symbolic);
}
