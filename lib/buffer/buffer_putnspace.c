

#include <stdlib.h>

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#else
#include "../alloc.h"
#endif /* HAVE_ALLOCA_H */

#include "../buffer.h"
#include "../byte.h"
#include "../str.h"

int
buffer_putnspace(buffer* b, int n) {
  if(n > 0) {
    int ret;
    char* space = (char*)
#ifdef HAVE_ALLOCA
        alloca(n);
#else
        alloc(n);
#endif
    byte_fill(space, n, ' ');
    ret = buffer_put(b, space, n);
#ifndef HAVE_ALLOCA
    alloc_free(space);
#endif
    return ret;
  }
  return 0;
}
