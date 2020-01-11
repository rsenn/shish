#include "../shell.h"
#include "../str.h"
#include "../stralloc.h"

#include <limits.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define START ((PATH_MAX + 1) >> 7)

/* read the link into a stralloc
 * ----------------------------------------------------------------------- */
int
shell_readlink(const char* path, stralloc* sa) {
#ifdef HAVE_READLINK
  /* do not allocate PATH_MAX from the beginning,
     most paths will be smaller */
  unsigned long n = (START ? START : 32);
  int sz;

  do {
    /* reserve some space */
    n <<= 1;
    stralloc_ready(sa, n);
    sz = readlink(path, sa->s, n);
    /* repeat until we have reserved enough space */
  } while(sz == n);

  /* now truncate to effective length */
  if(sz >= 0) {
    stralloc_trunc(sa, sz);
    return 0;
  }

  return -1;
#else
  stralloc_copys(sa, shell_basename((char*)path));
  return 0;
#endif
}
