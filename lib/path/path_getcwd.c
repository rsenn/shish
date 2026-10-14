#include "../byte.h"
#include "../stralloc.h"
#include "internal.h"
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
  size_t n = PATH_MAX, i;

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

  /* one separator style: every separator becomes PATHSEP_C */
  for(i = 0; i < sa->len; ++i)
    if(SEP(sa->s[i]))
      sa->s[i] = PATHSEP_C;

  return 1;
}
