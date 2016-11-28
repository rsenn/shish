#include "stralloc.h"
#include <stdlib.h>

#ifdef DEBUG

/* stralloc_readyplus is like stralloc_ready except that, if sa is
 * already allocated, stralloc_readyplus adds the current length of sa
 * to len. */
int stralloc_readyplusdebug(const char *file, unsigned int line, stralloc *sa, unsigned long len) {
  if (sa->s) {
    if (sa->len + len < len) return 0;	/* catch integer overflow */
    return stralloc_readydebug(file, line, sa, sa->len + len);
  } else
    return stralloc_readydebug(file, line, sa, len);
}
#endif /* defined(DEBUG) */