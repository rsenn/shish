#include "../str.h"
#include "../stralloc.h"

#define _POSIX_SOURCE 1
#include <limits.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LINUX_LIMITS_H
#include <linux/limits.h>
#endif

/* get current working directory into a stralloc
 * ----------------------------------------------------------------------- */
void
shell_getcwd(stralloc* sa, unsigned long start) {
  /* do not allocate PATH_MAX from the beginning,
     most paths will be smaller */
  size_t n = (start ? start : PATH_MAX / 16);

  do {
    /* reserve some space */
    stralloc_ready(sa, n);
    n += PATH_MAX / 8;
    /* repeat until we have reserved enough space */
  } while(getcwd(sa->s, sa->a) == NULL);

  /* now truncate to effective length */
  stralloc_trunc(sa, str_len(sa->s));
}
