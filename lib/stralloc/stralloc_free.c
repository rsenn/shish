#include "../shell.h"
#include "../stralloc.h"
#include <stdlib.h>

void
stralloc_free(stralloc* sa) {
  if(sa->s) {
    if(sa->a)
      alloc_free(sa->s);
  }
  sa->s = 0;
}
