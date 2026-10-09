#include "../path_internal.h"
#include "../windoze.h"

#if WINDOWS_NATIVE
#include <direct.h>
#else
#include <unistd.h>
#endif

#include <errno.h>
#include <limits.h>

/* get current working directory into a stralloc, NUL-terminated;
 * returns 1, or 0 with sa->len == 0 on failure
 * ----------------------------------------------------------------------- */
int
path_getcwd(stralloc* sa) {
  size_t n = PATH_MAX;
  char sep;

  sa->len = 0;

  for(;;) {
    if(!stralloc_ready(sa, n))
      return 0;

    if(getcwd(sa->s, sa->a))
      break;

    if(errno != ERANGE)
      return 0;

    n = sa->a * 2;
  }

  sa->len = str_len(sa->s);
  sep = path_getsep(sa->s);

  if(sep && sep != PATHSEP_C)
    stralloc_replacec(sa, sep, PATHSEP_C);

  return 1;
}
