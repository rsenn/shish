#include "stralloc.h"
#include <stdlib.h>

/* stralloc_readyplus is like stralloc_ready except that, if sa is
 * already allocated, stralloc_readyplus adds the current length of sa
 * to len. */
#ifdef DEBUG
int stralloc_readyplusdebug(const char *file, unsigned int line, stralloc *sa,unsigned long len)
#else
int stralloc_readyplus(stralloc *sa,unsigned long len)
#endif /* DEBUG */  
{
  if (sa->s) {
    if (sa->len + len < len) return 0;	/* catch integer overflow */
#ifdef DEBUG
    return stralloc_readydebug(file, line, sa, sa->len+len);
  } else
    return stralloc_readydebug(file, line, sa, len);
#else
    return stralloc_ready(sa,sa->len+len);
  } else
    return stralloc_ready(sa,len);
#endif /* DEBUG */
}
