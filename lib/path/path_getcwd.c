#include "../path_internal.h"
#include "../windoze.h"
#if WINDOWS_NATIVE
#include <direct.h>
#else
#include <unistd.h>
#endif

#include <limits.h>
/* get current working directory into a stralloc
 */
void
path_getcwd(stralloc* sa) {
  char *p, sep;
  stralloc_zero(sa);
  /* reserve some space */
  stralloc_ready(sa, PATH_MAX);
  /* repeat until we have reserved enough space */
  p = getcwd(sa->s, sa->a);
  sa->len = str_len(sa->s);
  sep = path_getsep(sa->s);
  if(sep && sep != PATHSEP_C)
    stralloc_replacec(sa, sep, PATHSEP_C);
}
