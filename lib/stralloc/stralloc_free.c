#include "stralloc.h"
#include <stdlib.h>

#ifndef DEBUG

void
stralloc_free(stralloc* sa) {
  if(sa->s) free(sa->s);
  sa->s = 0;
  sa->a = sa->len = 0;
}
#endif /* !defined(DEBUG) */