#include "../byte.h"
#include "../path_internal.h"
#include "../windoze.h"
#include <errno.h>
#ifndef ENAMETOOLONG
#define ENAMETOOLONG 91
#endif
extern int path_canonicalize(const char* path, stralloc* sa, int symbolic);
/* if the <path> is relative and <cwd> is non-null then it is prepended
 * to the path, so it will work like path_canonicalize, except that
 * relative paths will be resolved to absolute ones.
 */
int
path_realpath(const char* path, stralloc* sa, int symbolic, stralloc* cwd) {
  static stralloc tmpcwd;

  if(cwd == NULL) {
    path_getcwd(&tmpcwd);
    stralloc_nul(&tmpcwd);
    cwd = &tmpcwd;
  }

  /* if its not absolute on the first recursion level then make it so */
  if(!path_is_absolute(path) && sa->len == 0) {
    char buf[PATH_MAX + 1];
    /* check whether the name fits */
    size_t n;
    n = str_len(path);
    if(cwd->len + n + 1 > PATH_MAX) {
      errno = ENAMETOOLONG;
      return 0;
    }
    /* copy current dir */
    byte_copy(buf, cwd->len, cwd->s);
    buf[cwd->len] = PATHSEP_C;
    byte_copy(&buf[cwd->len + 1], n + 1, path);
    /* run canonicalize with the concatenated path */
    return path_canonicalize(buf, sa, symbolic);
  }
  return path_canonicalize(path, sa, symbolic);
}
